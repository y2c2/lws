/* qv : Loop
 * Copyright(c) 2016 y2c2 */

#ifndef QV_LOOP_H
#define QV_LOOP_H

#include "qv_types.h"
#include "qv_eventfd.h"
#include "qv_backend.h"
#include "qv_threadpool.h"
#include "qv_timerqueue.h"
#include "qv_taskqueue.h"

struct qv_loop
{
    qv_backend_t backend;
    qv_bool stop;
    qv_threadpool_t threadpool;
    qv_timerqueue_t timerqueue;
    qv_taskqueue_t taskqueue;

    int stashed_fd;
    qv_eventfd event;
    qv_eventfd event_trigger;

    int activated_nfds;
    int activated_idx;
    int *activated_fds;
};

int qv_loop_init(qv_loop_t *loop);
void qv_loop_close(qv_loop_t *loop);
int qv_loop_run(qv_loop_t *loop);
int qv_loop_stop(qv_loop_t *loop);
qv_backend_t *qv_loop_backend(qv_loop_t *loop);
qv_threadpool_t *qv_loop_threadpool(qv_loop_t *loop);
qv_taskqueue_t *qv_loop_taskqueue(qv_loop_t *loop);

#endif

