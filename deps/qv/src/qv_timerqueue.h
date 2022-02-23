/* qv : Timer Queue
 * Copyright(c) 2016 y2c2 */

#ifndef QV_TIMERQUEUE_H
#define QV_TIMERQUEUE_H

#include "qv_types.h"
#include "heap.h"

struct qv_timerqueue
{
    heap_t *timers;
};
typedef struct qv_timerqueue qv_timerqueue_t;

int qv_timerqueue_init(qv_timerqueue_t *timerqueue);
void qv_timerqueue_uninit(qv_timerqueue_t *timerqueue);

int qv_timerqueue_insert(qv_timerqueue_t *timerqueue, \
        qv_handle_t *handle);
int qv_timerqueue_remove(qv_timerqueue_t *timerqueue, \
        qv_handle_t *handle);

int qv_timerqueue_next_timeout(qv_timerqueue_t *timerqueue);

int qv_timerqueue_run(qv_timerqueue_t *timerqueue);

#endif

