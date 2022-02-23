/* qv : Process : UNIX
 * Copyright(c) 2016 y2c2 */

#ifndef QV_UNIXPROCESS_H
#define QV_UNIXPROCESS_H

#include "qv_loop.h"
#include "qv_handle.h"

/* How things work?
 * ================
 *
 * There are two different kind of way spawning
 * new processes.
 *
 * 1. Immediately
 * 2. Defered
 *
 *
 * Immediately
 * -----------
 * 1. Fork
 * 2. Child execute the file
 * 3. Parent create a thread to wait for the child
 * 4. Invoke vallback when child terminated
 *
 * Defered
 * -------
 * 1. Push a task
 * 2. When scheduling the task, do what should do in 
 *    immediately
 *
 * */

int qv_unixprocess_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options);

int qv_unixprocess_kill( \
        qv_handle_t *handle, int signum);

int qv_unixprocess_process( \
        qv_handle_t *handle);

#endif

