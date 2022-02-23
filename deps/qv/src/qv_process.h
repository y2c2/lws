/* qv : Process
 * Copyright(c) 2016 y2c2 */

#ifndef QV_PROCESS_H
#define QV_PROCESS_H

#include "qv_loop.h"

int qv_process_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options);
int qv_process_kill( \
        qv_handle_t *handle, int signum);

#endif

