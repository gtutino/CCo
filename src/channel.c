#include "../include/cco.h"
#include "channel.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

// ========================= Sender queue =========================

static void sender_enqueue(CCo_Channel *chan, Coroutine_Ctx *sender_ctx) {
    // Init the node
    Sender_Node *node = cco_malloc(sizeof(Sender_Node));
    node->ctx = sender_ctx;

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


CCo_Channel *cco_make_chan(size_t payload_size) {
    CCo_Channel *chan = cco_malloc(sizeof(CCo_Channel));
    chan->send_head = NULL;
    chan->send_tail = NULL;
    chan->recv_head = NULL;
    chan->recv_tail = NULL;
    chan->payload_size = payload_size;
    return chan;
}

// TODO: maybe we need a delete_chan ??

// Note: With multi threading all this function should be locked for the 'chan'
void cco_send(CCo_Channel *chan, void *data) {

    // If there are other senders in queue we go in queue, block coroutine and yield.
    if (chan->send_tail != NULL) {
        sender_enqueue(chan, &current_running->ctx);
        current_running->ctx.status = BLOCKED;
        cco_yield();
    }

    // If there is no reciver we do the same as above
    if (chan->recv_tail == NULL) {
        sender_enqueue(chan, &current_running->ctx);
        current_running->ctx.status = BLOCKED;
        cco_yield();
    }

    // An unblocked coroutine will get here.
    // If it was blocked in the first if then the second if condition is false
    // because there is a reciver in the queue that unblocked this actual coroutine.
    // If it was blocked in the second it resumes directly here.

    // Here we can send the data
    Reciver_Node *recv_node = reciver_dequeue(chan);
    memcpy(recv_node->dest, data, chan->payload_size);
    recv_node->ctx->status = NOT_RUNNING;
    free(recv_node);
}


void cco_recv(CCo_Channel *chan, void *dest) {

    // If there are other recivers in queue we go in queue, block coroutine and yield.
    if (chan->recv_tail != NULL) {
        reciver_enqueue(chan, &current_running->ctx, dest);
        current_running->ctx.status = BLOCKED;
        cco_yield();
    } else {
        // If the queue is empty we still enqueue because when whe
        // need to recive we have to be in the queue for the sender
        reciver_enqueue(chan, &current_running->ctx, dest);
    }

    // If there is no sender we do the same as above
    if (chan->send_tail == NULL) {
        current_running->ctx.status = BLOCKED;
        cco_yield();
    }

    // Here we unblock and remove the sender from the queue
    Sender_Node *send_node = sender_dequeue(chan);
    send_node->ctx->status = NOT_RUNNING;
    free(send_node);

    // We will be unblocker after the sender will send the data
    current_running->ctx.status = BLOCKED;
    cco_yield();
}
