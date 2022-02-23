/* qv : Process : NT
 * Copyright(c) 2016 y2c2 */

#ifndef QV_NTPROCESS_H
#define QV_NTPROCESS_H

#include "qv_loop.h"
#include "qv_handle.h"

int qv_ntprocess_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options);

int qv_ntprocess_kill( \
        qv_handle_t *handle, int signum);

int qv_ntrocess_process( \
        qv_handle_t *handle);

#endif


