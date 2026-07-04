#ifndef CCO_CHANNEL_H
#define CCO_CHANNEL_H

#include "../include/cco.h"
#include "coroutine.h"

typedef struct Sender_Node Sender_Node
struct Sender_Node {
    Sender_Node *next;
    Coroutine_Ctx *sender;
};

typedef struct Reciver_Node Reciver_Node
struct Reciver_Node {
    Reciver_Node *next;
    Coroutine_Ctx *reciver;
    void *dest;
};

struct CCo_Channel {
    Sender_Node *head;
    Reciver_Node *head;
    size_t payload_size;
};

#endif
