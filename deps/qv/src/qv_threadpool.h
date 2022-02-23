/* qv : Thread Pool
 * Copyright(c) 2016 y2c2 */

#ifndef QV_THREAD_POOL_H
#define QV_THREAD_POOL_H

#include "qv_types.h"
#include "qv_eventfd.h"
#include "queue.h"
#include "qv_thread.h"

struct qv_threadpool;
typedef struct qv_threadpool qv_threadpool_t;

/* Task to be dispatched to a worker */
typedef void *(*qv_thread_task_t)(void *data);

/* Deal with the processed data of a task */
typedef void (*qv_thread_cont_t)(void *data);

/* Task */
struct qv_task
{
    /* The routine of the task */
    qv_thread_task_t thread_task;
    /* The data to be processed */
    void *data;

    /* The returned data */
    void *res;
    /* The routine to process the returned data */
    qv_thread_cont_t cont;
};
typedef struct qv_task qv_task_t;

/* Worker */
struct qv_worker
{
    /* Thread */
    qv_thread_t thread;
    /* If the thread has been initialized */
    qv_bool thread_initalized;

    /* Thread pool */ 
    qv_threadpool_t *threadpool;
};
typedef struct qv_worker qv_worker_t;

/* Thread Pool */
struct qv_threadpool
{
    /* Tell workers to stop running */
    volatile qv_bool stop;

    /* Worker */
    struct
    {
        qv_worker_t *workers;

        volatile qv_size_t num_total;
        volatile qv_size_t num_working;
        qv_mutex_t num_lock;
    } workers;

    /* Tasks */
    struct
    {
        int has_task;
        qv_mutex_t mutex_has_task;
        qv_cond_t cond_has_task;
        queue_t *pending;

        queue_t *finished;
    } tasks;

    qv_eventfd event_trigger;
};

int qv_threadpool_init( \
        qv_threadpool_t *pool, \
        qv_eventfd event_trigger, \
        qv_size_t worker_num);
int qv_threadpool_uninit(qv_threadpool_t *pool);

int qv_threadpool_dispatch( \
        qv_threadpool_t *pool, \
        qv_thread_task_t task, \
        void *data, \
        qv_thread_cont_t cont);

int qv_threadpool_retrieve(qv_threadpool_t *pool);

#endif


