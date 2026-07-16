#include "../include/cco.h"
#include "coroutine.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

typedef struct Node Node;
struct Node {
    Node *next;
    Node *prev;
    Coroutine_Ctx *ctx;
    void *data;
};

typedef struct {
    void *data;
    size_t count;
    size_t capacity;
    size_t head;
    size_t tail;
} Buffer;

struct CCo_Channel {
    Node *send_head;
    Node *send_tail;
    Node *recv_head;
    Node *recv_tail;
    size_t payload_size;
    pthread_mutex_t lock;
    Buffer buf;
};


// =========================== Queue =============================

static void node_enqueue(Node **head, Node **tail, Coroutine_Ctx *ctx, void *data) {
    // Init the node
    Node *node = cco_malloc(sizeof(Node));
    node->ctx = ctx;
    node->data = data;

    // Push into the queue
    if (*head == NULL && *tail == NULL) {
        node->next = NULL;
        node->prev = NULL;
        *head = node;
        *tail = node;
    } else {
        node->next = *head;
        node->prev = NULL;
        (*head)->prev = node;
        *head = node;
    }
}

// Returns NULL if the queue is empty
static Node *node_dequeue(Node **head, Node **tail) {
    if (*head == NULL) {
        return NULL;
    }
    else if (*head == *tail) {
        Node *to_return = *head;
        *head = NULL;
        *tail = NULL;
        return to_return;
    }
    else {
        Node *to_return = *tail;
        Node *new_tail = (*tail)->prev;
        *tail = new_tail;
        new_tail->next = NULL;
        return to_return;
    }
}

// ======================== // Queue // ==========================


// ====================== Send - Recv queues ======================

static inline void sender_enqueue(CCo_Channel *chan, Coroutine_Ctx *sender_ctx, void *data) {
    node_enqueue(&chan->send_head, &chan->send_tail, sender_ctx, data);
}

static inline Node *sender_dequeue(CCo_Channel *chan) {
    return node_dequeue(&chan->send_head, &chan->send_tail);
}

static inline void reciver_enqueue(CCo_Channel *chan, Coroutine_Ctx *reciver_ctx, void *dest) {
    node_enqueue(&chan->recv_head, &chan->recv_tail, reciver_ctx, dest);
}

static inline Node *reciver_dequeue(CCo_Channel *chan) {
    return node_dequeue(&chan->recv_head, &chan->recv_tail);
}

// =================== // Send - Recv queues // ===================


// =========================== Buffer =============================

static int buf_enqueue(CCo_Channel *chan, void *data) {

    // Buffer full
    if (chan->buf.count == chan->buf.capacity) {
        return 1;
    }

    memcpy(
        ((uint8_t*)chan->buf.data) + (chan->buf.tail*chan->payload_size),
        data,
        chan->payload_size
    );

    chan->buf.count++;
    chan->buf.tail = (chan->buf.tail + 1) % chan->buf.capacity;
    return 0;
}

static void *buf_dequeue(CCo_Channel *chan) {
    if (chan->buf.count == 0) {
        return NULL;
    }

    void *data = ((uint8_t*)chan->buf.data) + (chan->buf.head*chan->payload_size);
    chan->buf.count--;
    chan->buf.head = (chan->buf.head + 1) % chan->buf.capacity;
    return data;
}

// ======================== // Buffer // ==========================


CCo_Channel *cco_make_chan(size_t payload_size, size_t buffer_capacity) {
    CCo_Channel *chan = cco_malloc(sizeof(CCo_Channel));
    chan->send_head = NULL;
    chan->send_tail = NULL;
    chan->recv_head = NULL;
    chan->recv_tail = NULL;
    chan->payload_size = payload_size;
    pthread_mutex_init(&chan->lock, NULL);

    if (buffer_capacity > 0) {
        chan->buf.data = cco_malloc(payload_size*buffer_capacity);
        chan->buf.count = 0;
        chan->buf.capacity = buffer_capacity;
        chan->buf.head = 0;
        chan->buf.tail = 0;
    }
    else {
        chan->buf.data = NULL;
    }

    return chan;
}

void cco_free_chan(CCo_Channel *chan) {
    if (chan->send_head != NULL || chan->recv_head != NULL) {
        fprintf(stderr, "[ERROR]: Freeing a non-empty channel!\n");
        exit(1);
    }
    if (chan->buf.data != NULL) {
        free(chan->buf.data);
    }
    free(chan);
}


void cco_send(CCo_Channel *chan, void *data) {
    pthread_mutex_lock(&chan->lock);

    // If there is a reciver ready we send the data and return
    if (chan->recv_tail != NULL) {
        Node *recv_node = reciver_dequeue(chan);
        memcpy(recv_node->data, data, chan->payload_size);
        recv_node->ctx->status = NOT_RUNNING;
        free(recv_node);

        pthread_mutex_unlock(&chan->lock);
        return;
    }

    // If there is a non-full buffer we put there the data
    if (chan->buf.data != NULL) {
        if (buf_enqueue(chan, data) == 0) {
            pthread_mutex_unlock(&chan->lock);
            return;
        }
    }

    // If there is no reciver and the buffer is full we block and enqueue
    sender_enqueue(chan, &current_running->ctx, data);
    current_running->ctx.status = BLOCKED;

    pthread_mutex_unlock(&chan->lock);
    cco_yield();
}


void cco_recv(CCo_Channel *chan, void *dest) {
    pthread_mutex_lock(&chan->lock);

    // If there is something in the buffer we pull from that
    if (chan->buf.data != NULL) {
        void *data = buf_dequeue(chan);
        if (data != NULL) {
            memcpy(dest, data, chan->payload_size);
            pthread_mutex_unlock(&chan->lock);
            return;
        }
    }

    // If there is a sender ready we get the data and return
    if (chan->send_tail != NULL) {
        Node *send_node = sender_dequeue(chan);
        memcpy(dest, send_node->data, chan->payload_size);
        send_node->ctx->status = NOT_RUNNING;
        free(send_node);

        pthread_mutex_unlock(&chan->lock);
        return;
    }

    // If there is no sender and the buffer is empty we block and enqueue
    reciver_enqueue(chan, &current_running->ctx, dest);
    current_running->ctx.status = BLOCKED;

    pthread_mutex_unlock(&chan->lock);
    cco_yield();
}
