/* qv : Timer
 * Copyright(c) 2016 y2c2 */

#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_timer.h"
#include "qv_loop.h"
#include "qv_utils.h"

int qv_timer_init(qv_loop_t *loop, qv_handle_t *handle)
{
    handle->type = QV_HANDLE_TYPE_TIMER;
    handle->mode = QV_BACKEND_NONE;
    handle->loop = loop;
    handle->u.timer.state = QV_TIMER_STATE_AFTER;
    handle->u.timer.timer = NULL;
    handle->u.timer.after = 0;
    handle->u.timer.repeat = 0;
    handle->data = NULL;
    handle->data_dtor = NULL;

    return 0;
}

int qv_timer_set(qv_handle_t *handle, \
        qv_timer_cb_t cb, \
        qv_interval_t after, \
        qv_interval_t repeat)
{
    handle->u.timer.timer = cb;
    handle->u.timer.after = after;
    handle->u.timer.repeat = repeat;

    return 0;
}

int qv_timer_start(qv_handle_t *handle)
{
    qv_loop_t *loop = handle->loop;

    return qv_timerqueue_insert(&loop->timerqueue, handle);
}

int qv_timer_stop(qv_handle_t *handle)
{
    qv_loop_t *loop = handle->loop;

    return qv_timerqueue_remove(&loop->timerqueue, \
            handle);
}

int qv_timer_again(qv_handle_t *handle)
{
    handle->u.timer.again_flag = qv_true;

    return 0;
}

int qv_timer_process(qv_handle_t *handle)
{
    qv_loop_t *loop = handle->loop;
    qv_timer_t *timer = &handle->u.timer;
    
    /* Reset again flag */
    timer->again_flag = qv_false;

    /* Invoke callback */
    handle->u.timer.timer(handle);

    if (timer->again_flag != qv_false)
    {
        timer->state = QV_TIMER_STATE_REPEAT;
        if (qv_timerqueue_insert(&loop->timerqueue, handle) != 0)
        { return -1; }
    }

    return 0;
}

