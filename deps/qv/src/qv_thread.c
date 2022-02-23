/* qv : Thread
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include "qv_pthread.h"
#elif defined(QV_PLATFORM_NT)
#include "qv_ntthread.h"
#endif

#include "qv_types.h"
#include "qv_thread.h"


int qv_thread_init(qv_thread_t *thd, qv_thread_routine_t routine, void *data)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_create(thd, routine, data);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_create(thd, routine, data);
#endif
}

int qv_thread_cancel(qv_thread_t *thd)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cancel(thd);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cancel(thd);
#endif
}

int qv_thread_join(qv_thread_t *thd, void **retval)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_join(thd, retval);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_join(thd, retval);
#endif
}

int qv_mutex_init(qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_mutex_init(mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_mutex_init(mtx);
#endif
}

int qv_mutex_uninit(qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_mutex_uninit(mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_mutex_uninit(mtx);
#endif
}

int qv_mutex_lock(qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_mutex_lock(mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_mutex_trylock(mtx);
#endif
}

int qv_mutex_trylock(qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_mutex_lock(mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_mutex_trylock(mtx);
#endif
}

int qv_mutex_unlock(qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_mutex_unlock(mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_mutex_unlock(mtx);
#endif
}

int qv_cond_init(qv_cond_t *cond)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_init(cond);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_init(cond);
#endif
}

int qv_cond_uninit(qv_cond_t *cond)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_uninit(cond);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_uninit(cond);
#endif
}

int qv_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_wait(cond, mtx);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_wait(cond, mtx);
#endif
}

int qv_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_timedwait(cond, mtx, interval);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_timedwait(cond, mtx, interval);
#endif
}

int qv_cond_signal(qv_cond_t *cond)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_signal(cond);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_signal(cond);
#endif
}

int qv_cond_broadcast(qv_cond_t *cond)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_pthread_cond_broadcast(cond);
#elif defined(QV_PLATFORM_NT)
    return qv_ntthread_cond_broadcast(cond);
#endif
}
