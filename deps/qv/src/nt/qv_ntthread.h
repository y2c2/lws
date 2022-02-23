/* qv : NT Thread
 * Copyright(c) 2016 y2c2 */

#ifndef QV_NTTHREAD_H
#define QV_NTTHREAD_H

#include <Windows.h>

typedef HANDLE qv_thread_t;
typedef CRITICAL_SECTION qv_mutex_t;
typedef CONDITION_VARIABLE qv_cond_t;

/* Thread */
int qv_ntthread_create(qv_thread_t *thd, \
	void *(*proto_callback)(void *), void *proto_data);
int qv_ntthread_cancel(qv_thread_t *thd);
int qv_ntthread_join(qv_thread_t *thd, void **retval);

/* Mutex */
int qv_ntthread_mutex_init(qv_mutex_t *mtx);
int qv_ntthread_mutex_uninit(qv_mutex_t *mtx);
int qv_ntthread_mutex_lock(qv_mutex_t *mtx);
int qv_ntthread_mutex_trylock(qv_mutex_t *mtx);
int qv_ntthread_mutex_unlock(qv_mutex_t *mtx);

/* Cond */
int qv_ntthread_cond_init(qv_cond_t *cond);
int qv_ntthread_cond_uninit(qv_cond_t *cond);
int qv_ntthread_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx);
int qv_ntthread_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval);
int qv_ntthread_cond_signal(qv_cond_t *cond);
int qv_ntthread_cond_broadcast(qv_cond_t *cond);

#endif

