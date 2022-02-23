/* qv : NT Thread
 * Copyright(c) 2016 y2c2 */

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#include <Windows.h>
#include <process.h>
#include "qv_allocator.h"
#include "qv_ntthread.h"

struct ntthread_bridge_stub
{
	void *(*proto_callback)(void *);
	void *proto_data;
};

typedef struct ntthread_bridge_stub ntthread_bridge_stub_t;

static ntthread_bridge_stub_t *ntthread_bridge_stub_new(\
	void *(*proto_callback)(void *), void *proto_data)
{
	ntthread_bridge_stub_t *new_stub = (ntthread_bridge_stub_t *)qv_malloc(\
		sizeof(ntthread_bridge_stub_t));
	if (new_stub == NULL) return NULL;
	new_stub->proto_callback = proto_callback;
	new_stub->proto_data = proto_data;
	return new_stub;
}

void ntthread_bridge_stub_destroy(ntthread_bridge_stub_t *stub)
{
	qv_free(stub);
}

DWORD ntthread_bridge_stub_start_routine(void *bridge_data)
{
	ntthread_bridge_stub_t *stub = bridge_data;

	stub->proto_callback(stub->proto_data);

	ntthread_bridge_stub_destroy(stub);

	return 0;
}

/* Thread */
int qv_ntthread_create(qv_thread_t *thd, \
        void *(*proto_callback)(void *), void *proto_data)
{
	ntthread_bridge_stub_t *new_stub = NULL;
	HANDLE new_thread;
	
	if ((new_stub = ntthread_bridge_stub_new(proto_callback, proto_data)) == NULL)
	{ return -1; }

	new_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ntthread_bridge_stub_start_routine, new_stub, 0, NULL);
	if (new_thread == INVALID_HANDLE_VALUE)
	{
		ntthread_bridge_stub_destroy(new_stub);
		return -1;
	}
	*thd = new_thread;
	return 0;
}

int qv_ntthread_cancel(qv_thread_t *thd)
{
	(void)thd;
	/* TerminateThread(*thd, 0); */
	return 0;
}

int qv_ntthread_join(qv_thread_t *thd, void **retval)
{
	WaitForSingleObject(*thd, INFINITE);
	*retval = 0;
	return 0;
}

/* Mutex */
int qv_ntthread_mutex_init(qv_mutex_t *mtx)
{
	InitializeCriticalSection(mtx);
	return 0;
}

int qv_ntthread_mutex_uninit(qv_mutex_t *mtx)
{
	(void)mtx;
	return 0;
}

int qv_ntthread_mutex_lock(qv_mutex_t *mtx)
{
	EnterCriticalSection(mtx);
	return 0;
}

int qv_ntthread_mutex_trylock(qv_mutex_t *mtx)
{
	return TryEnterCriticalSection(mtx) == FALSE ? 1 : 0;
}

int qv_ntthread_mutex_unlock(qv_mutex_t *mtx)
{
	LeaveCriticalSection(mtx);
	return 0;
}

int qv_ntthread_cond_init(qv_cond_t *cond)
{
	InitializeConditionVariable(cond);
	return 0;
}

int qv_ntthread_cond_uninit(qv_cond_t *cond)
{
	(void)cond;
	return 0;
}

int qv_ntthread_cond_wait(qv_cond_t *cond, qv_mutex_t *mtx)
{
	SleepConditionVariableCS(cond, mtx, INFINITE);
	return 0;
}

int qv_ntthread_cond_timedwait(qv_cond_t *cond, qv_mutex_t *mtx, int interval)
{
	SleepConditionVariableCS(cond, mtx, interval);
	return 0;
}

int qv_ntthread_cond_signal(qv_cond_t *cond)
{
	WakeConditionVariable(cond);
	return 0;
}

int qv_ntthread_cond_broadcast(qv_cond_t *cond)
{
	WakeAllConditionVariable(cond);
	return 0;
}
