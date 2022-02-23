/* qv : Timer
 * Copyright(c) 2016 y2c2 */

#ifndef QV_TIMER_H
#define QV_TIMER_H

#include "qv_handle.h"

int qv_timer_init(qv_loop_t *loop, qv_handle_t *handle);
int qv_timer_set(qv_handle_t *handle, \
        qv_timer_cb_t cb, \
        qv_interval_t after, \
        qv_interval_t repeat);
int qv_timer_start(qv_handle_t *handle);
int qv_timer_stop(qv_handle_t *handle);
int qv_timer_again(qv_handle_t *handle);

int qv_timer_process(qv_handle_t *handle);

#endif

