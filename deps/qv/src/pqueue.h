/* Packet Queue
 * Copyright(c) 2016-2018 y2c2 */

#ifndef PQUEUE_H
#define PQUEUE_H

#include "qv_types.h"
#include "qv_socketaddr.h"

typedef qv_size_t pqueue_size_t;

struct pqueue_node;
typedef struct pqueue_node pqueue_node_t;
struct pqueue;
typedef struct pqueue pqueue_t;

typedef void *(*pqueue_malloc_cb_t)(pqueue_size_t size);
typedef void (*pqueue_free_cb_t)(void *ptr);

struct pqueue_node
{
    char *data;
    qv_size_t len;
    qv_socketaddr_t addr;

    pqueue_node_t *prev, *next;
};

struct pqueue
{
    pqueue_node_t *head, *tail;

    pqueue_malloc_cb_t cb_malloc;
    pqueue_free_cb_t cb_free;
    qv_size_t count;
};

pqueue_t *pqueue_new( \
        pqueue_malloc_cb_t cb_malloc, \
        pqueue_free_cb_t cb_free);
void pqueue_destroy(pqueue_t *q);

int pqueue_push(pqueue_t *q, char *data, qv_size_t len, qv_socketaddr_t *addr);
int pqueue_pop(pqueue_t *q, char **data, qv_size_t *len, qv_socketaddr_t *addr);
qv_size_t pqueue_count(pqueue_t *q);


#endif


