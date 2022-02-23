/* qv : User Task
 * Copyright(c) 2016 y2c2 */

#include "qv_loop.h"
#include "qv_handle.h"
#include "qv_taskqueue.h"
#include "qv_usertask.h"

int qv_usertask_init( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_usertask_cb_t cb, \
        qv_handle_t *data)
{
    handle->type = QV_HANDLE_TYPE_USERTASK;
    handle->mode = QV_BACKEND_NONE;
    handle->loop = loop;
    handle->u.usertask.cb = cb;
    handle->u.usertask.data = data;
    handle->data = NULL;
    handle->data_dtor = NULL;

    /* Push the task */
    if (qv_taskqueue_push_back(qv_loop_taskqueue(loop), handle) != 0)
    { return -1; }

    /* Inform the loop */
    qv_eventfd_write(loop->event_trigger, 1);

    return 0;
}

