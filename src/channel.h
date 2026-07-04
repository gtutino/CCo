#ifndef CCO_CHANNEL_H
#define CCO_CHANNEL_H

#include "../include/cco.h"
#include "coroutine.h"

typedef struct Sender_Node Sender_Node
struct Sender_Node {
    Sender_Node *next;
    Sender_Node *prev;
    Coroutine_Ctx *sender;
};

typedef struct Reciver_Node Reciver_Node
struct Reciver_Node {
    Reciver_Node *next;
    Reciver_Node *prev;
    Coroutine_Ctx *reciver;
    void *dest;
};

struct CCo_Channel {
    Sender_Node *send_head;
    Sender_Node *send_tail;
    Reciver_Node *recv_head;
    Sender_Node *recv_tail;
    size_t payload_size;
};

#endif
