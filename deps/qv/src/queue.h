/* Queue
 * Copyright(c) 2016-2018 y2c2 */

#ifndef QUEUE_H
#define QUEUE_H

#include "qv_types.h"
#include "qv_thread.h"

typedef qv_size_t queue_size_t;

struct queue_node;
typedef struct queue_node queue_node_t;
struct queue;
typedef struct queue queue_t;

typedef void *(*queue_malloc_cb_t)(queue_size_t size);
typedef void (*queue_free_cb_t)(void *ptr);

struct queue_node
{
    void *data;

    queue_node_t *prev, *next;
};

struct queue
{
    queue_node_t *head, *tail;

    queue_malloc_cb_t cb_malloc;
    queue_free_cb_t cb_free;

    qv_mutex_t mtx;
};

queue_t *queue_new( \
        queue_malloc_cb_t cb_malloc, \
        queue_free_cb_t cb_free);
void queue_destroy(queue_t *q);

qv_bool queue_empty(queue_t *q);

enum
{
    QUEUE_PUSH_OK = 0,
    QUEUE_PUSH_FAIL = -1,
};
int queue_push(queue_t *q, void *data);

enum
{
    QUEUE_POP_OK = 0,
    QUEUE_POP_FAIL = -1,
    QUEUE_POP_EMPTY = -2,
};
int queue_pop(queue_t *q, void **data);

enum
{
    QUEUE_TRYPUSH_OK = 0,      /* Success */
    QUEUE_TRYPUSH_AGAIN = -1,  /* Locked, try again */
    QUEUE_TRYPUSH_MEM = -2,    /* Out of memory */
};
int queue_trypush(queue_t *q, void *data);

enum
{
    QUEUE_TRYPOP_OK = 0,      /* Success */
    QUEUE_TRYPOP_AGAIN = -1,  /* Locked, try again */
    QUEUE_TRYPOP_EMPTY = -2,  /* Query empty */
};
int queue_trypop(queue_t *q, void **data);


#endif

