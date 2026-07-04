#include "../include/cco.h"
#include "channel.h"
#include "common.h"

// CCo_Channel *cco_make_chan(size_t payload_size);
// void cco_send(CCo_Channel *chan, void *data);
// void cco_recv(CCo_Channel *chan, void *data);

static void sender_enqueue(CCo_Channel *chan, Coroutine_Ctx *sender) {
    // Init the node
    Sender_Node *node = cco_malloc(sizeof(Sender_Node));
    node->sender = sender;

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
    }
    else {
        Sender_Node *to_return = chan->send_tail;
        Sender_Node *new_tail = chan->send-tail->prev;
        chan->send_tail = new_tail;
        new_tail->next = NULL;
    }

    return to_return;
}

void cco_send(CCo_Channel *chan, void *data) {

    // With multi threading all this function should be locked for the 'chan'

    if (chan->send_tail != NULL) {
        // TODO: Go in queue and BLOCK coroutine and yield with different ctx save
        // (by saving the real rip of this function and the real rsp, rbp)
    }

    if (chan->recv_tail == NULL) {
        // TODO: Same as above
    }

    // TODO:
    // If the coroutine blocks then it should resume here, this can
    // be done by setting the ctx rip to here before blocking maybe

    // Here we are the first and there is a coroutine that is reciving
    // TODO: copy the data to dest, unqueue recv and unblock it
}
