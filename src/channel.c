#include "../include/cco.h"
#include "coroutine.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// TODO: here can be used just a 'Node' struct
typedef struct Sender_Node Sender_Node;
struct Sender_Node {
    Sender_Node *next;
    Sender_Node *prev;
    Coroutine_Ctx *ctx;
    void *data;
};

typedef struct Reciver_Node Reciver_Node;
struct Reciver_Node {
    Reciver_Node *next;
    Reciver_Node *prev;
    Coroutine_Ctx *ctx;
    void *dest;
};

typedef struct {
    void *data;
    size_t count;
    size_t capacity;
    size_t head;
    size_t tail;
} Buffer;

struct CCo_Channel {
    Sender_Node *send_head;
    Sender_Node *send_tail;
    Reciver_Node *recv_head;
    Reciver_Node *recv_tail;
    size_t payload_size;
    pthread_mutex_t lock;
    Buffer buf;
};


// ========================= Sender queue =========================

static void sender_enqueue(CCo_Channel *chan, Coroutine_Ctx *sender_ctx, void *data) {
    // Init the node
    Sender_Node *node = cco_malloc(sizeof(Sender_Node));
    node->ctx = sender_ctx;
    node->data = data;

    // Push into the queue
    if (chan->send_head == NULL && chan->send_tail == NULL) {
        node->next = NULL;
        node->prev = NULL;
        chan->send_head = node;
        chan->send_tail = node;
    } else {
        node->next = chan->send_head;
        node->prev = NULL;
        chan->send_head->prev = node;
        chan->send_head = node;
    }
}

// Returns NULL if the queue is empty
static Sender_Node *sender_dequeue(CCo_Channel *chan) {
    if (chan->send_head == NULL) {
        return NULL;
    }
    else if (chan->send_head == chan->send_tail) {
        Sender_Node *to_return = chan->send_head;
        chan->send_head = NULL;
        chan->send_tail = NULL;
        return to_return;
    }
    else {
        Sender_Node *to_return = chan->send_tail;
        Sender_Node *new_tail = chan->send_tail->prev;
        chan->send_tail = new_tail;
        new_tail->next = NULL;
        return to_return;
    }
}

// ====================== // Sender queue // ======================


// ======================== Reciver queue =========================

static void reciver_enqueue(CCo_Channel *chan, Coroutine_Ctx *reciver_ctx, void *dest) {
    // Init the node
    Reciver_Node *node = cco_malloc(sizeof(Reciver_Node));
    node->ctx = reciver_ctx;
    node->dest = dest;

    // Push into the queue
    if (chan->recv_head == NULL && chan->recv_tail == NULL) {
        node->next = NULL;
        node->prev = NULL;
        chan->recv_head = node;
        chan->recv_tail = node;
    } else {
        node->next = chan->recv_head;
        node->prev = NULL;
        chan->recv_head->prev = node;
        chan->recv_head = node;
    }
}

// Returns NULL if the queue is empty
static Reciver_Node *reciver_dequeue(CCo_Channel *chan) {
    if (chan->recv_head == NULL) {
        return NULL;
    }
    else if (chan->recv_head == chan->recv_tail) {
        Reciver_Node *to_return = chan->recv_head;
        chan->recv_head = NULL;
        chan->recv_tail = NULL;
        return to_return;
    }
    else {
        Reciver_Node *to_return = chan->recv_tail;
        Reciver_Node *new_tail = chan->recv_tail->prev;
        chan->recv_tail = new_tail;
        new_tail->next = NULL;
        return to_return;
    }
}

// ===================== // Reciver queue // ======================


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
        fprintf(stderr, "[WARNING]: Freeing a non-empty channel!\n");
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
        Reciver_Node *recv_node = reciver_dequeue(chan);
        memcpy(recv_node->dest, data, chan->payload_size);
        recv_node->ctx->status = NOT_RUNNING;
        free(recv_node);

        pthread_mutex_unlock(&chan->lock);
        return;
    }

    // If there is no reciver we block and enqueue
    sender_enqueue(chan, &current_running->ctx, data);
    current_running->ctx.status = BLOCKED;

    pthread_mutex_unlock(&chan->lock);
    cco_yield();
}


void cco_recv(CCo_Channel *chan, void *dest) {
    pthread_mutex_lock(&chan->lock);

    // If there is a sender ready we get the data and return
    if (chan->send_tail != NULL) {
        Sender_Node *send_node = sender_dequeue(chan);
        memcpy(dest, send_node->data, chan->payload_size);
        send_node->ctx->status = NOT_RUNNING;
        free(send_node);

        pthread_mutex_unlock(&chan->lock);
        return;
    }

    // If there is no sender we block and enqueue
    reciver_enqueue(chan, &current_running->ctx, dest);
    current_running->ctx.status = BLOCKED;

    pthread_mutex_unlock(&chan->lock);
    cco_yield();
}
