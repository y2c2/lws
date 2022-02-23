/* qv : Thread Pool
 * Copyright(c) 2016 y2c2 */

#include <string.h> 

#include "queue.h"
#include "qv_types.h"
#include "qv_eventfd.h"
#include "qv_allocator.h"
#include "qv_thread.h"
#include "qv_threadpool.h"
#include "qv_utils.h"

/* Task */
static qv_task_t *qv_task_new( \
    qv_thread_task_t thread_task, \
    void *data, \
    qv_thread_cont_t cont)
{
    qv_task_t *new_task;

    if ((new_task = (qv_task_t *)qv_malloc(sizeof(qv_task_t))) == NULL)
    { return NULL; }
    new_task->thread_task = thread_task;
    new_task->data = data;
    new_task->res = NULL;
    new_task->cont = cont;

    return new_task;
}

static void qv_task_destroy(qv_task_t *task)
{
    qv_free(task);
}

/* Worker */
static void *qv_threadpool_worker_routine(void *data)
{
    int ret_queueop;
    qv_worker_t *worker = (qv_worker_t *)data;
    qv_threadpool_t *threadpool = worker->threadpool;
    qv_task_t *new_task = NULL;

    qv_mutex_lock(&threadpool->workers.num_lock);
    threadpool->workers.num_total++;
    qv_mutex_unlock(&threadpool->workers.num_lock);

    for (;;)
    {

        /* Block until get a task */
        qv_mutex_lock(&threadpool->tasks.mutex_has_task);
        while (threadpool->tasks.has_task != 1)
        { qv_cond_wait(&threadpool->tasks.cond_has_task, &threadpool->tasks.mutex_has_task); }
        threadpool->tasks.has_task = 0;
        qv_mutex_unlock(&threadpool->tasks.mutex_has_task);

        /* Something happened, either has job or gonna stop */
        if (threadpool->stop == qv_true) break;
        else
        {
            ret_queueop = queue_trypop(threadpool->tasks.pending, (void **)&new_task);
            if (ret_queueop == 0)
            {
                qv_mutex_lock(&threadpool->workers.num_lock);
                threadpool->workers.num_working++;
                qv_mutex_unlock(&threadpool->workers.num_lock);

                /* Good, we get the job */
                /* Fetched a new task, execute the task */
                new_task->res = new_task->thread_task(new_task->data);

                /* Put the new task into pending tasks queue */
                for (;;)
                {
                    /* Cancel the dispatching when got the signal */
                    if (threadpool->stop != qv_false) break;
                    ret_queueop = queue_trypush(threadpool->tasks.finished, new_task);
                    if (ret_queueop == 0) break;
                    else if (ret_queueop == QUEUE_TRYPUSH_AGAIN) continue;
                    else if (ret_queueop == QUEUE_TRYPUSH_MEM) continue;
                    else
                    {
                        /* Unknown error, probably a BUG? Anyway, destroy the task
                         * TODO: RETHINK ABOUT IT */
                    }
                }

                /* Clean task */
                new_task = NULL;

                /* Inform the loop */
                qv_eventfd_write(threadpool->event_trigger, 1);

                qv_mutex_lock(&threadpool->workers.num_lock);
                threadpool->workers.num_working--;
                qv_mutex_unlock(&threadpool->workers.num_lock);
            }
            else if (ret_queueop == QUEUE_TRYPOP_AGAIN)
            {
                /* Lock busy, wait */
            }
            else if (ret_queueop == QUEUE_TRYPOP_EMPTY)
            {
                /* Currently no job */
            }
        }
    }

    qv_mutex_lock(&threadpool->workers.num_lock);
    threadpool->workers.num_total--;
    qv_mutex_unlock(&threadpool->workers.num_lock);

    return NULL;
}

static int qv_worker_init(qv_threadpool_t *threadpool, qv_worker_t *worker)
{
    if (qv_thread_init(&worker->thread, qv_threadpool_worker_routine, worker) != 0)
    { return -1; }
    worker->threadpool = threadpool;
    worker->thread_initalized = qv_true;

    return 0;
}

static int qv_worker_uninit(qv_worker_t *worker)
{
    /* How to stop a thread? */
    if (worker->thread_initalized == qv_true)
    {
        qv_thread_cancel(&worker->thread);
        worker->thread_initalized = qv_false;
    }

    return 0;
}

static int qv_worker_join(qv_worker_t *worker)
{
    void *retval;

    if (worker->thread_initalized == qv_false)
    { return 0; }
    
    qv_thread_join(&worker->thread, &retval);

    (void)retval;

    return 0;
}

int qv_threadpool_init( \
        qv_threadpool_t *pool, \
        qv_eventfd event_trigger, \
        qv_size_t workers_num)
{
    int ret = 0;
    qv_size_t i;

    /* Event FD */
    pool->event_trigger = event_trigger;

    /* Allocate memory for workers */
    if ((pool->workers.workers = (qv_worker_t *)qv_malloc( \
                    sizeof(qv_worker_t) * workers_num)) == NULL)
    { return -1; }
    pool->workers.num_total = 0;
    pool->workers.num_working = 0;

    /* Clean */
    memset(pool->workers.workers, 0, sizeof(qv_worker_t) * workers_num);
    for (i = 0; i != workers_num; i++)
    {
        pool->workers.workers[i].thread_initalized = qv_false;
        pool->workers.workers[i].threadpool = pool;
    }
    pool->tasks.pending= NULL;
    pool->tasks.finished = NULL;
    pool->stop = qv_false;

    /* The queues MUST be initialized before the workds
     * due to they could be accessed by the threading of workers */

    /* Pending tasks */
    if ((pool->tasks.pending = queue_new(qv_malloc, qv_free)) == NULL)
    { ret = -1; goto fail; }
    /* Finished tasks */
    if ((pool->tasks.finished = queue_new(qv_malloc, qv_free)) == NULL)
    { ret = -1; goto fail; }

    /* Initialize lock */
    pool->tasks.has_task = 0;
    qv_mutex_init(&pool->tasks.mutex_has_task);
    qv_cond_init(&pool->tasks.cond_has_task);

    /* Initialize each worker */
    qv_mutex_init(&pool->workers.num_lock);
    for (i = 0; i != workers_num; i++)
    {
        if (qv_worker_init(pool, &pool->workers.workers[i]) != 0)
        { ret = -1; goto fail; }
    }
    while (pool->workers.num_total != workers_num) { }

    goto done;
fail:
    qv_threadpool_uninit(pool);
done:
    return ret;
}

int qv_threadpool_uninit(qv_threadpool_t *pool)
{
    qv_size_t i;
    qv_size_t count = pool->workers.num_total;

    /* Signal the stop command */ 
    pool->stop = qv_true;

    /* Wake up all workers */ 
    qv_mutex_lock(&pool->tasks.mutex_has_task);
    pool->tasks.has_task = 1;
    qv_cond_broadcast(&pool->tasks.cond_has_task);
    qv_mutex_unlock(&pool->tasks.mutex_has_task);

    /* Release memory of workers */
    if (pool->workers.workers != NULL)
    {
        /* Confirm all worker threadings been terminated */
        for (i = 0; i != count; i++)
        {
            qv_worker_join(&pool->workers.workers[i]);
            qv_worker_uninit(&pool->workers.workers[i]);
        }
        qv_free(pool->workers.workers);
    }

    /* Pending tasks */
    if (pool->tasks.pending != NULL)
    {
        queue_destroy(pool->tasks.pending);
    }
    /* Finished tasks */
    if (pool->tasks.finished != NULL)
    {
        queue_destroy(pool->tasks.finished);
    }

    qv_mutex_uninit(&pool->tasks.mutex_has_task);
    qv_cond_uninit(&pool->tasks.cond_has_task);

    return 0;
}

int qv_threadpool_dispatch( \
        qv_threadpool_t *pool, \
        qv_thread_task_t task, \
        void *data, \
        qv_thread_cont_t cont)
{
    int ret = 0, ret_queueop;
    qv_task_t *new_task;

    if ((new_task = qv_task_new(task, data, cont)) == NULL)
    { ret = -1; goto fail; }

    /* Put the new task into pending tasks queue */
    for (;;)
    {
        /* Cancel the dispatching when got the signal */
        if (pool->stop != qv_false) break;

        ret_queueop = queue_trypush(pool->tasks.pending, new_task);
        if (ret_queueop == 0) break;
        else if (ret_queueop == QUEUE_TRYPUSH_AGAIN) continue;
        else if (ret_queueop == QUEUE_TRYPUSH_MEM)
        {
            ret = -1;
            goto fail;
        }
        else
        {
            /* Unknown error, probably a BUG? */
            ret = -1;
            goto fail;
        }
    }
    pool->tasks.has_task = 1;
    qv_cond_signal(&pool->tasks.cond_has_task);

    goto done;
fail:
    if (new_task != NULL) { qv_task_destroy(new_task); }
done:
    return ret;
}

int qv_threadpool_retrieve(qv_threadpool_t *pool)
{
    int ret_queueop;
    qv_task_t *new_task;

    for (;;)
    {
        ret_queueop = queue_trypop(pool->tasks.finished, (void **)&new_task);
        if (ret_queueop == 0)
        {
            new_task->cont(new_task->data);

            qv_task_destroy(new_task);
            new_task = NULL;
        }
        else if (ret_queueop == QUEUE_TRYPOP_AGAIN)
        {
        }
        else if (ret_queueop == QUEUE_TRYPOP_EMPTY)
        {
            break;
        }
    }

    return 0;
}

