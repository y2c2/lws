/* qv : POSIX Thread
 * Copyright(c) 2016 y2c2 */

#ifndef QV_PTHREAD_H
#define QV_PTHREAD_H

#include "pthread.h"

typedef pthread_t qv_thread_t;
typedef pthread_mutex_t qv_mutex_t;
typedef pthread_cond_t qv_cond_t;

/* Thread */
int qv_pthread_create(qv_thread_t *thd, \
        void *(*callback)(void *), void *data);
int qv_pthread_cancel(qv_thread_t *thd);
int qv_pthread_join(qv_thread_t *thd, void **retval);

/* Mutex */
int qv_pthread_mutex_init(qv_mutex_t *mtx);
int qv_pthread_mutex_uninit(qv_mutex_t *mtx);
int qv_pthread_mutex_lock(qv_mutex_t *mtx);
int qv_pthread_mutex_trylock(qv_mutex_t *mtx);
int qv_pthread_mutex_unlock(qv_mutex_t *mtx);

/* Cond */
int qv_pthread_cond_init(qv_cond_t *cond);
int qv_pthread_cond_uninit(qv_cond_t *cond);
int qv_pthread_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx);
int qv_pthread_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval);
int qv_pthread_cond_signal(qv_cond_t *cond);
int qv_pthread_cond_broadcast(qv_cond_t *cond);

#endif

