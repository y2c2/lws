/* qv : Task Queue
 * Copyright(c) 2016 y2c2 */

#ifndef QV_TASKQUEUE_H
#define QV_TASKQUEUE_H

#include "qv_types.h"
#include "qv_handle.h"
#include "queue.h"

struct qv_taskqueue
{
    qv_bool stop;
    queue_t *tasks;
};
typedef struct qv_taskqueue qv_taskqueue_t;

int qv_taskqueue_init(qv_taskqueue_t *taskqueue);
int qv_taskqueue_init_autofree(qv_taskqueue_t *taskqueue);
void qv_taskqueue_uninit(qv_taskqueue_t *taskqueue);

int qv_taskqueue_push_back(qv_taskqueue_t *taskqueue, \
        qv_handle_t *handle);

int qv_taskqueue_empty(qv_taskqueue_t *taskqueue);
int qv_taskqueue_run(qv_taskqueue_t *taskqueue);


#endif

