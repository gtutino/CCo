#define _XOPEN_SOURCE 600
#include "../include/cco.h"
#include "coroutine.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>

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
    Buffer buf;
    pthread_mutex_t lock;
};

typedef struct {
    cco_chan_func func;
    CCo_Channel *chan;
    void *data;
} Chan_Args;


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

static inline void receiver_enqueue(CCo_Channel *chan, Coroutine_Ctx *receiver_ctx, void *dest) {
    node_enqueue(&chan->recv_head, &chan->recv_tail, receiver_ctx, dest);
}

static inline Node *receiver_dequeue(CCo_Channel *chan) {
    return node_dequeue(&chan->recv_head, &chan->recv_tail);
}

// =================== // Send - Recv queues // ===================


// =========================== Buffer =============================

static bool buf_enqueue(CCo_Channel *chan, void *data) {

    // Buffer full
    if (chan->buf.count == chan->buf.capacity) {
        return false;
    }

    memcpy(
        ((uint8_t*)chan->buf.data) + (chan->buf.tail*chan->payload_size),
        data,
        chan->payload_size
    );

    chan->buf.count++;
    chan->buf.tail = (chan->buf.tail + 1) % chan->buf.capacity;
    return true;
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

    // Recursive mutex needed for the cco_select function
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&chan->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    // Buffer init
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
    pthread_mutex_destroy(&chan->lock);
    free(chan);
}


void cco_send(CCo_Channel *chan, void *data) {
    pthread_mutex_lock(&chan->lock);

    // If there is a non-full buffer we put there the data
    if (chan->buf.data != NULL) {
        if (buf_enqueue(chan, data)) {
            pthread_mutex_unlock(&chan->lock);
            return;
        }
    }

    // If there is a receiver ready we send the data and return
    if (chan->recv_tail != NULL) {
        Node *recv_node = receiver_dequeue(chan);
        memcpy(recv_node->data, data, chan->payload_size);
        recv_node->ctx->status = NOT_RUNNING;
        free(recv_node);

        pthread_mutex_unlock(&chan->lock);
        return;
    }


    // If there is no receiver and the buffer is full we block and enqueue
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

            // If there is some sender in queue now it can put data in the buffer
            if (chan->send_tail != NULL) {
                Node *send_node = sender_dequeue(chan);
                buf_enqueue(chan, send_node->data);
                send_node->ctx->status = NOT_RUNNING;
                free(send_node);
            }

            pthread_mutex_unlock(&chan->lock);
            return;
        }
    }

    // This case is reachable only if there is no buffer.
    // We cannot pull from queue when there is a buffer
    // because we need to take in account the order of the messages.
    else {
        // If there is a sender ready we get the data and return
        if (chan->send_tail != NULL) {
            Node *send_node = sender_dequeue(chan);
            memcpy(dest, send_node->data, chan->payload_size);
            send_node->ctx->status = NOT_RUNNING;
            free(send_node);

            pthread_mutex_unlock(&chan->lock);
            return;
        }
    }

    // If there is no sender or the buffer is empty we block and enqueue
    receiver_enqueue(chan, &current_running->ctx, dest);
    current_running->ctx.status = BLOCKED;

    pthread_mutex_unlock(&chan->lock);
    cco_yield();
}


// This two functions checks if a channel can send or can recieve,
// so basically if there is a waiting receiver/sender
// (or if there is space in the buffer).
//
// [NOTE]
// If the channel can send/receive the mutex is NOT
// unlocked to prevent other coroutines to send/receive messages.
// So the caller MUST take account of unlocking.
static bool chan_can_send(CCo_Channel *chan) {
    if (pthread_mutex_trylock(&chan->lock) == 0) {
        if (chan->recv_tail != NULL) {
            return true;
        }
        if (chan->buf.data != NULL && chan->buf.count < chan->buf.capacity) {
            return true;
        }
        pthread_mutex_unlock(&chan->lock);
        return false;
    }
    return false;
}

static bool chan_can_recv(CCo_Channel *chan) {
    if (pthread_mutex_trylock(&chan->lock) == 0) {
        if (chan->send_tail != NULL) {
            return true;
        }
        if (chan->buf.data != NULL && chan->buf.count > 0) {
            return true;
        }
        pthread_mutex_unlock(&chan->lock);
        return false;
    }
    return false;
}

size_t cco_select_impl(cco_chan_func first_fun, ...) {
    va_list args;
    va_start(args, first_fun);

    Chan_Args ca[16]; // TODO: in the readme tell about this max 16.
    int ca_index = 0;

    cco_chan_func func = first_fun;
    CCo_Channel *chan;
    void *data;
    bool chan_available;

    while (true) {
        chan = va_arg(args, CCo_Channel*);
        data = va_arg(args, void*);

        if (func == cco_send) {
            chan_available = chan_can_send(chan);
        }
        else if (func == cco_recv) {
            chan_available = chan_can_recv(chan);
        }
        else {
            fprintf(stderr, "[ERROR]: Unreachable.\n");
            exit(1);
        }

        if (chan_available) {
            if (ca_index < 16) {
                ca[ca_index] = (Chan_Args) {
                    .func = func,
                    .chan = chan,
                    .data = data
                };
                ca_index++;
            }
            else {
                fprintf(stderr, "[ERROR]: Max num of functions is 16.\n");
                exit(1);
            }
        }

        func = va_arg(args, cco_chan_func);

        if (func == NULL) {
            va_end(args);
            break;
        }
    }

    if (ca_index > 0) {
        int choosen_channel_index = rand() % ca_index;

        // Unlock all the other channels
        for (int i = 0; i < ca_index; i++) {
            if (i != choosen_channel_index) {
                pthread_mutex_unlock(&ca[i].chan->lock);
            }
        }

        // Perform the request
        func = ca[choosen_channel_index].func;
        chan = ca[choosen_channel_index].chan;
        data = ca[choosen_channel_index].data;

        func(chan, data);
        pthread_mutex_unlock(&chan->lock);
        return choosen_channel_index + 1;
    }

    // No channels available
    return 0;
}
