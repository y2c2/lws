/* qv : Timer Queue
 * Copyright(c) 2016 y2c2 */

#include "qv_types.h"
#include "qv_allocator.h"
#include "qv_handle.h"
#include "chrono.h"
#include "heap.h"
#include "qv_timer.h"
#include "qv_timerqueue.h"

static int heap_timer_interval_cmp( \
        heap_node_data_t *key1, \
        heap_node_data_t *key2)
{
    if (key1->u64 == key2->u64) return 0;
    return (key1->u64 < key2->u64) ? -1 : 1;
}

int qv_timerqueue_init(qv_timerqueue_t *timerqueue)
{
    if ((timerqueue->timers = heap_new( \
                    qv_malloc, qv_free, \
                    heap_timer_interval_cmp)) == NULL)
    { return -1; }

    return 0;
}

void qv_timerqueue_uninit(qv_timerqueue_t *timerqueue)
{
    if (timerqueue->timers != NULL)
    { heap_destroy(timerqueue->timers); }
}

int qv_timerqueue_insert(qv_timerqueue_t *timerqueue, \
        qv_handle_t *handle)
{
    heap_node_data_t key;
    heap_node_data_t value;
    chrono_time_point_t tp_now;

    /* Get time point of now */
    if (chrono_now(&tp_now) != 0) return -1;

    /* Key: time point of timeout */
    switch (handle->u.timer.state)
    {
        case QV_TIMER_STATE_AFTER:
            key.u64 = tp_now + handle->u.timer.after;
            break;
        case QV_TIMER_STATE_REPEAT:
            key.u64 = tp_now + handle->u.timer.repeat;
            break;
    }
    value.ptr = handle;

    if (heap_insert(timerqueue->timers, &key, &value) != 0)
    { return -1; }

    return 0;
}

static qv_bool qv_timerqueue_heap_search_fn_by_handle( \
        heap_node_t *heap_node, void *data)
{
    return (heap_node->value.ptr == data) ? qv_true : qv_false;
}

int qv_timerqueue_remove(qv_timerqueue_t *timerqueue, \
        qv_handle_t *handle)
{
    heap_node_t *node = heap_search_by( \
            timerqueue->timers, \
            qv_timerqueue_heap_search_fn_by_handle, \
            handle);
    if (node == NULL) return -1;

    heap_remove(timerqueue->timers, node);

    return 0;
}

int qv_timerqueue_next_timeout(qv_timerqueue_t *timerqueue)
{
    chrono_time_point_t tp_timer_timeout, tp_now;
    heap_node_t *root;

    /* Get the earliest timeout timer */
    root = heap_root(timerqueue->timers);
    if (root == NULL)
    {
        /* Infinity */
        return -1;
    }

    /* Get time point of now */
    if (chrono_now(&tp_now) != 0) return -1;
    /* Get the timeout time point of the timer */
    tp_timer_timeout = (chrono_time_point_t)(root->key.u64);

    /* Now is earlier than time out of timer */
    if (tp_now < tp_timer_timeout)
    {
        return (int)(tp_timer_timeout - tp_now);
    }

    /* Already timeout */
    return 0;
}

int qv_timerqueue_run(qv_timerqueue_t *timerqueue)
{
    chrono_time_point_t tp_timer_timeout, tp_now;
    heap_node_t *root;
    qv_handle_t *handle;

    for (;;)
    {
        /* Try to get the most possibly timeout timer */
        root = heap_root(timerqueue->timers);
        if (root == NULL) return 0;

        /* Get time point of now */
        if (chrono_now(&tp_now) != 0) return -1;
        /* Get the timeout time point of the timer */
        tp_timer_timeout = (chrono_time_point_t)(root->key.u64);

        /* Now is earlier than time out of timer */
        if (tp_now < tp_timer_timeout) { return 0; }

        /* Pop the timer */
        handle = (qv_handle_t *)heap_root(timerqueue->timers)->value.ptr;
        heap_remove_root(timerqueue->timers);

        /* Process the timer */
        qv_timer_process(handle);
    }
}

