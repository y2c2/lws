/* Queue
 * Copyright(c) 2016-2018 y2c2 */

#include "qv_thread.h"
#include "queue.h"

static queue_node_t *queue_node_new(queue_t *q, void *data)
{
    queue_node_t *new_queue_node;

    new_queue_node = (queue_node_t *)q->cb_malloc(sizeof(queue_node_t));
    if (new_queue_node == NULL) return NULL;

    new_queue_node->data = data;
    new_queue_node->prev = new_queue_node->next = NULL;

    return new_queue_node;
}

static void queue_node_destroy(queue_t *q, queue_node_t *node)
{
    q->cb_free(node);
}

queue_t *queue_new( \
        queue_malloc_cb_t cb_malloc, \
        queue_free_cb_t cb_free)
{
    queue_t *new_queue;

    new_queue = (queue_t *)cb_malloc(sizeof(queue_t));
    if (new_queue == NULL) return NULL;
    new_queue->cb_malloc = cb_malloc;
    new_queue->cb_free = cb_free;
    new_queue->head = new_queue->tail = NULL;

    /* Initialize the lock */
    if (qv_mutex_init(&new_queue->mtx) != 0)
    { goto fail; }

    goto done;
fail:
    if (new_queue != NULL)
    {
        queue_destroy(new_queue);
        new_queue = NULL;
    }
done:
    return new_queue;
}

void queue_destroy(queue_t *q)
{
    queue_node_t *node_cur, *node_next;

    node_cur = q->head;
    while (node_cur != NULL)
    {
        node_next = node_cur->next;
        queue_node_destroy(q, node_cur);
        node_cur = node_next;
    }

    qv_mutex_uninit(&q->mtx);

    q->cb_free(q);
}

qv_bool queue_empty(queue_t *q)
{
    qv_bool ret;
    if (qv_mutex_trylock(&q->mtx) != 0)
    {
        ret = q->head == q->tail ? qv_true : qv_false;
        return ret;
    }
    ret = q->head == q->tail ? qv_true : qv_false;
    qv_mutex_unlock(&q->mtx);
    return ret;
}

int queue_push(queue_t *q, void *data)
{
    queue_node_t *new_node;

    if (qv_mutex_lock(&q->mtx) != 0) { return QUEUE_PUSH_FAIL; }

    /* acquired */

    if ((new_node = queue_node_new(q, data)) == NULL)
    {
        /* Out of memory, release the lock and return */
        qv_mutex_unlock(&q->mtx);
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

    /* Success, release the lock */
    qv_mutex_unlock(&q->mtx);
    return 0;
}

int queue_pop(queue_t *q, void **data)
{
    queue_node_t *next;

    if (qv_mutex_lock(&q->mtx) != 0)
    {
        /* Failed to acquire the lock */
        return QUEUE_POP_FAIL;
    }

    if (q->head == NULL)
    {
        /* Query empty */
        qv_mutex_unlock(&q->mtx);
        return QUEUE_POP_EMPTY;
    }

    *data = q->head->data;

    next = q->head->next;
    if (next == NULL)
    {
        q->tail = NULL;
    }
    else
    {
        next->prev = NULL;
    }

    q->cb_free(q->head);
    q->head = next;

    /* Success, release the lock */
    qv_mutex_unlock(&q->mtx);
    return 0;
}

int queue_trypush(queue_t *q, void *data)
{
    queue_node_t *new_node;

    if (qv_mutex_trylock(&q->mtx) != 0)
    {
        /* Failed to acquire the lock */
        return QUEUE_TRYPUSH_AGAIN;
    }

    /* acquired */

    if ((new_node = queue_node_new(q, data)) == NULL)
    {
        /* Out of memory, release the lock and return */
        qv_mutex_unlock(&q->mtx);
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

    /* Success, release the lock */
    qv_mutex_unlock(&q->mtx);
    return 0;
}

int queue_trypop(queue_t *q, void **data)
{
    queue_node_t *next;

    if (qv_mutex_trylock(&q->mtx) != 0)
    {
        /* Failed to acquire the lock */
        return QUEUE_TRYPOP_AGAIN;
    }

    if (q->head == NULL)
    {
        /* Query empty */
        qv_mutex_unlock(&q->mtx);
        return QUEUE_TRYPOP_EMPTY;
    }

    *data = q->head->data;

    next = q->head->next;
    if (next == NULL)
    {
        q->tail = NULL;
    }
    else
    {
        next->prev = NULL;
    }

    q->cb_free(q->head);
    q->head = next;

    /* Success, release the lock */
    qv_mutex_unlock(&q->mtx);
    return 0;
}

