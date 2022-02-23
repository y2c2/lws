/* Packet Queue
 * Copyright(c) 2016-2018 y2c2 */

#include "qv_libc.h"
#include "pqueue.h"

static pqueue_node_t *pqueue_node_new( \
        pqueue_t *q, char *data, qv_size_t len, qv_socketaddr_t *addr)
{
    pqueue_node_t *new_queue_node;

    new_queue_node = (pqueue_node_t *)q->cb_malloc(sizeof(pqueue_node_t));
    if (new_queue_node == NULL) return NULL;

    qv_memcpy(new_queue_node->data, data, len);
    new_queue_node->len = len;
    qv_memcpy(&new_queue_node->addr, addr, sizeof(qv_socketaddr_t));
    new_queue_node->prev = new_queue_node->next = NULL;

    return new_queue_node;
}

static void pqueue_node_destroy(pqueue_t *q, pqueue_node_t *node)
{
    q->cb_free(node);
}

pqueue_t *pqueue_new( \
        pqueue_malloc_cb_t cb_malloc, \
        pqueue_free_cb_t cb_free)
{
    pqueue_t *new_pqueue;

    new_pqueue = (pqueue_t *)cb_malloc(sizeof(pqueue_t));
    if (new_pqueue == NULL) return NULL;
    new_pqueue->cb_malloc = cb_malloc;
    new_pqueue->cb_free = cb_free;
    new_pqueue->head = new_pqueue->tail = NULL;
    new_pqueue->count = 0;

    return new_pqueue;
}

void pqueue_destroy(pqueue_t *q)
{
    pqueue_node_t *node_cur, *node_next;

    node_cur = q->head;
    while (node_cur != NULL)
    {
        node_next = node_cur->next;
        pqueue_node_destroy(q, node_cur);
        node_cur = node_next;
    }

    q->cb_free(q);
}

int pqueue_push(pqueue_t *q, char *data, qv_size_t len, qv_socketaddr_t *addr)
{
    pqueue_node_t *new_node;

    if ((new_node = pqueue_node_new(q, data, len, addr)) == NULL)
    {
        /* Out of memory, return */
        return -1;
    }

    if (q->head == NULL)
    {
        q->head = q->tail = new_node;
    }
    else
    {
        new_node->prev = q->tail;
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->count++;

    return 0;
}

int pqueue_pop(pqueue_t *q, char **data, qv_size_t *len, qv_socketaddr_t *addr)
{
    pqueue_node_t *next;

    if (q->head == NULL)
    {
        return -1;
    }

    *data = q->head->data;
    *len = q->head->len;
    qv_memcpy(addr, &q->head->addr, sizeof(qv_socketaddr_t));

    next = q->head->next;
    if (next == NULL)
    {
        q->tail = NULL;
    }
    else
    {
        next->prev = NULL;
    }
    q->count--;

    q->cb_free(q->head);
    q->head = next;

    return 0;
}

qv_size_t pqueue_count(pqueue_t *q)
{
    return q->count;
}

