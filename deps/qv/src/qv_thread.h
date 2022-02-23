/* qv : Thread
 * Copyright(c) 2016 y2c2 */

#ifndef QV_THREAD_H
#define QV_THREAD_H

#include "qv_config.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include "qv_pthread.h"
#elif defined(QV_PLATFORM_MACOS)
#include "qv_pthread.h"
#elif defined(QV_PLATFORM_NT)
#include "qv_ntthread.h"
#endif

/* Prototype of thread routine */
typedef void *(*qv_thread_routine_t)(void *data);

int qv_thread_init(qv_thread_t *thd, qv_thread_routine_t routine, void *data);
int qv_thread_cancel(qv_thread_t *thd);
int qv_thread_join(qv_thread_t *thd, void **retval);

int qv_mutex_init(qv_mutex_t *mtx);
int qv_mutex_uninit(qv_mutex_t *mtx);
int qv_mutex_lock(qv_mutex_t *mtx);
int qv_mutex_trylock(qv_mutex_t *mtx);
int qv_mutex_unlock(qv_mutex_t *mtx);

int qv_cond_init(qv_cond_t *cond);
int qv_cond_uninit(qv_cond_t *cond);
int qv_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx);
int qv_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval);
int qv_cond_signal(qv_cond_t *cond);
int qv_cond_broadcast(qv_cond_t *cond);

#endif

