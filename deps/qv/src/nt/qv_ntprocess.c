/* qv : Process : NT
 * Copyright(c) 2016 y2c2 */

#include <process.h>
#include "qv_allocator.h"
#include "qv_ntprocess.h"
#include "qv_libc.h"
#include "qv_usertask.h"

typedef struct nt_process_stub
{
	PROCESS_INFORMATION processInformation;
	STARTUPINFO startupInfo;
} nt_process_stub_t;

nt_process_stub_t *nt_process_stub_new(void)
{
	nt_process_stub_t *new_stub = (nt_process_stub_t *)qv_malloc(sizeof(nt_process_stub_t));
	if (new_stub == NULL) return NULL;
	qv_memset(&new_stub->processInformation, 0, sizeof(PROCESS_INFORMATION));
	qv_memset(&new_stub->startupInfo, 0, sizeof(STARTUPINFO));
	return new_stub;
}

void nt_process_stub_destroy(nt_process_stub_t *stub)
{
	qv_free(stub);
}

static void *qv_process_thread_routine(void *data)
{
	qv_handle_t *handle = (qv_handle_t *)data;
	qv_loop_t *loop = handle->loop;
	qv_process_t *process = &handle->u.process;
	nt_process_stub_t *process_stub = (nt_process_stub_t *)process->process_stub;
	DWORD ret;

	for (;;)
	{
		ret = WaitForSingleObject(process_stub->processInformation.hProcess, 500);
		if (ret == WAIT_TIMEOUT)
		{
			if (loop->stop == 0) continue;
			else
			{
				TerminateProcess(process_stub->processInformation.hProcess, 0);
				break;
			}
		}
	}

	return NULL;
}

static void qv_process_thread_cont(void *data)
{
	qv_handle_t *handle = (qv_handle_t *)data;
	qv_process_t *process = &handle->u.process;
	nt_process_stub_t *process_stub = (nt_process_stub_t *)process->process_stub;
	process->options.exit_cb(handle, process->return_code, process->term_signal);
	CloseHandle(process_stub->processInformation.hProcess);
	CloseHandle(process_stub->processInformation.hThread);
	nt_process_stub_destroy(process_stub);
	process->process_stub = NULL;
}

static int qv_ntprocess_exec(\
	qv_handle_t *handle)
{
	qv_loop_t *loop = handle->loop;
	qv_threadpool_t *threadpool = qv_loop_threadpool(loop);
	qv_process_t *process = &handle->u.process;
	qv_process_options_t *options = &process->options;
	qv_u32 flags = options->flags;
	nt_process_stub_t *new_process_stub;

	(void)flags;

	if ((new_process_stub = nt_process_stub_new()) == NULL)
	{
		return -1;
	}
	new_process_stub->startupInfo.cb = sizeof(new_process_stub->startupInfo);

	/* Execute file */
	if (CreateProcess(NULL, options->file, \
		NULL, NULL, FALSE, \
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, \
		options->envp, options->wd, \
		&new_process_stub->startupInfo, \
		&new_process_stub->processInformation) == FALSE)
	{
		/* Failed to execute the executable file */
		nt_process_stub_destroy(new_process_stub);
		exit(-1);
	}

	handle->u.process.state = QV_PROCESS_STATE_RUNNING;
	process->pid = (qv_pid_t)(new_process_stub->processInformation.dwProcessId);

	/* Dispatch a new thread to monitor the child */
	if (qv_threadpool_dispatch(\
		threadpool, \
		qv_process_thread_routine, \
		handle, \
		qv_process_thread_cont) != 0)
	{
		CloseHandle(new_process_stub->processInformation.hProcess);
		CloseHandle(new_process_stub->processInformation.hThread);
		nt_process_stub_destroy(new_process_stub);
		return -1;
	}

	process->process_stub = new_process_stub;

	return 0;
}

static void qv_ntprocess_exec_task(\
	qv_handle_t *handle, void *data)
{
	qv_handle_t *handle_process = (qv_handle_t *)data;
	qv_ntprocess_exec(handle_process);
	qv_free(handle);
}

int qv_ntprocess_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options)
{
	qv_handle_t *defered_task = NULL;

	handle->loop = loop;
	handle->type = QV_HANDLE_TYPE_PROCESS;
	handle->u.process.state = QV_PROCESS_STATE_SPAWNING;
	handle->u.process.pid = 0;
	handle->u.process.return_code = 0;
	handle->u.process.term_signal = 0;
	handle->u.process.process_stub = NULL;
	qv_memcpy(&handle->u.process.options, \
		options, \
		sizeof(qv_process_options_t));

	/* Defer */
	if (options->defer == qv_true)
	{
		/* Add a task to delay the creating */
		defered_task = (qv_handle_t *)qv_malloc(sizeof(qv_handle_t));
		if (defered_task == NULL) return -1;

		if (qv_usertask_init(\
			loop, defered_task, qv_ntprocess_exec_task, handle) != 0)
		{
			qv_free(defered_task);
			return -1;
		}

		return 0;
	}

	return qv_ntprocess_exec(handle);
}

int qv_ntprocess_kill( \
        qv_handle_t *handle, int signum)
{
	qv_process_t *process = &handle->u.process;
	nt_process_stub_t *process_stub = (nt_process_stub_t *)process->process_stub;

	(void)signum;
	TerminateProcess(process_stub->processInformation.hProcess, 0);
	CloseHandle(process_stub->processInformation.hProcess);
	CloseHandle(process_stub->processInformation.hThread);

	return 0;
}

int qv_ntrocess_process( \
        qv_handle_t *handle)
{
	qv_ntprocess_exec(handle);

	return 0;
}

