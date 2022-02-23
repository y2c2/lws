/* qv : User Task
 * Copyright(c) 2016 y2c2 */

#ifndef QV_USERTASK_H
#define QV_USERTASK_H

#include "qv_handle.h"

int qv_usertask_init( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_usertask_cb_t cb, \
        qv_handle_t *data);


#endif

