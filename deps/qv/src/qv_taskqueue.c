/* qv : Task Queue
 * Copyright(c) 2016 y2c2 */

#include "qv_types.h"
#include "qv_allocator.h"
#include "qv_handle.h"
#include "queue.h"
#include "qv_taskqueue.h"

int qv_taskqueue_init(qv_taskqueue_t *taskqueue)
{
    if ((taskqueue->tasks = queue_new( \
                    qv_malloc, qv_free)) == NULL)
    { return -1; }
    taskqueue->stop = qv_false;

    return 0;
}

void qv_taskqueue_uninit(qv_taskqueue_t *taskqueue)
{
    if (taskqueue->tasks != NULL)
    { queue_destroy(taskqueue->tasks); }
}

int qv_taskqueue_push_back(qv_taskqueue_t *taskqueue, \
        qv_handle_t *handle)
{
    int ret_queueop;

    /* Put the new task into pending tasks queue */
    for (;;)
    {
        /* Cancel the dispatching when got the signal */
        if (taskqueue->stop != qv_false) break;

        ret_queueop = queue_trypush( \
                taskqueue->tasks, handle);
        if (ret_queueop == 0) break;
        else if (ret_queueop == QUEUE_TRYPUSH_AGAIN) continue;
        else if (ret_queueop == QUEUE_TRYPUSH_MEM) continue;
        else { }
    }

    return 0;
}

int qv_taskqueue_empty(qv_taskqueue_t *taskqueue)
{
    return queue_empty(taskqueue->tasks) == qv_true ? 1 : 0;
}

int qv_taskqueue_run(qv_taskqueue_t *taskqueue)
{
    int queueop;
    qv_handle_t *handle;

    for (;;)
    {
        queueop = queue_trypop(taskqueue->tasks, (void **)&handle);
        if (queueop == QUEUE_TRYPOP_EMPTY)
        { break; }
        else if (queueop == QUEUE_TRYPOP_AGAIN)
        { continue; }
        else if (queueop == QUEUE_TRYPOP_OK)
        {
            handle->u.usertask.cb(handle, handle->u.usertask.data);
        }
        else { }
    }

    return 0;
}

