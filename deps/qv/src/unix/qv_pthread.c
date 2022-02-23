/* qv : POSIX Thread
 * Copyright(c) 2016 y2c2 */

#include <errno.h>
#include <pthread.h>

#include "qv_pthread.h"

int qv_pthread_create(qv_thread_t *thd, void *(*callback)(void *), void *data)
{
    pthread_attr_t attr;

    if (pthread_attr_init(&attr) != 0) { return -1; }
    if (pthread_create(thd, &attr, callback, data) != 0) { return -1; }
    if (pthread_attr_destroy(&attr) != 0) { return -1; }

    return 0;
}

int qv_pthread_cancel(qv_thread_t *thd)
{
    return pthread_cancel(*thd);
}

int qv_pthread_join(qv_thread_t *thd, void **retval)
{
    return pthread_join(*thd, retval);
}

/* Mutex */

int qv_pthread_mutex_init(qv_mutex_t *mtx)
{
    int ret = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    ret = pthread_mutex_init(mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return ret;
}

int qv_pthread_mutex_uninit(qv_mutex_t *mtx)
{
    return pthread_mutex_destroy(mtx);
}

int qv_pthread_mutex_lock(qv_mutex_t *mtx)
{
    return pthread_mutex_lock(mtx);
}

int qv_pthread_mutex_trylock(qv_mutex_t *mtx)
{
    if (pthread_mutex_trylock(mtx) == EBUSY)
    { return 1; }

    return 0;
}

int qv_pthread_mutex_unlock(qv_mutex_t *mtx)
{
    return pthread_mutex_unlock(mtx);
}

/* Cond */
int qv_pthread_cond_init(qv_cond_t *cond)
{
    return pthread_cond_init(cond, NULL);
}

int qv_pthread_cond_uninit(qv_cond_t *cond)
{
    return pthread_cond_destroy(cond);
}

int qv_pthread_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx)
{
    return pthread_cond_wait(cond, mtx);
}

int qv_pthread_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = interval * 1000;
    return pthread_cond_timedwait(cond, mtx, &ts);
}

int qv_pthread_cond_signal(qv_cond_t *cond)
{
    return pthread_cond_signal(cond);
}

int qv_pthread_cond_broadcast(qv_cond_t *cond)
{
    return pthread_cond_broadcast(cond);
}

