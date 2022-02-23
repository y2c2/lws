/* qv : Process : UNIX
 * Copyright(c) 2016 y2c2 */

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_libc.h"
#include "qv_unixprocess.h"
#include "qv_usertask.h"


static void *qv_process_thread_routine(void *data)
{
    qv_handle_t *handle = (qv_handle_t *)data;
    qv_loop_t *loop = handle->loop;
    qv_process_t *process = &handle->u.process;
    int status = 0;
    struct rusage rusage;
    qv_pid_t pid;

    for (;;)
    {
        if (loop->stop != 0) break;

        pid = wait4(process->pid, &status, WNOHANG, &rusage);
        if (pid != 0)
        {
            process->state = QV_PROCESS_STATE_TERMINATED;
            break;
        }
    }

    return NULL;
}

static void qv_process_thread_cont(void *data)
{
    qv_handle_t *handle = (qv_handle_t *)data;
    qv_process_t *process = &handle->u.process;
    process->options.exit_cb(handle, process->return_code, process->term_signal);
}

static int qv_unixprocess_exec( \
        qv_handle_t *handle)
{
    int ret;
    qv_loop_t *loop = handle->loop;
    qv_threadpool_t *threadpool = qv_loop_threadpool(loop);
    qv_process_t *process = &handle->u.process;
    qv_process_options_t *options = &process->options;
    qv_u32 flags = options->flags;
    extern char **environ;

    ret = fork();
    if (ret == -1)
    {
        /* Failed */
        process->state = QV_PROCESS_STATE_TERMINATED;
        process->pid = 0;
        process->term_signal = 0;
        process->return_code = -1;

        if (process->options.defer != qv_false)
        {
            /* Deferred */
            process->options.exit_cb(handle, -1, 0);
        }

        return -1;
    }
    else if (ret == 0)
    {
        /* Child Process */ 

        /* Set environment */
        environ = options->envp;

        /* Set GID */
        if (flags & QV_PROCESS_OPTIONS_SET_GID)
        { if (setgid(options->gid) != 0) exit(-1); }

        if (flags & QV_PROCESS_OPTIONS_SET_UID)
        { if (setuid(options->uid) != 0) exit(-1); }

        /* Execute file */
        if (execvp(options->file, options->argv) != 0)
        {
            /* Failed to execute the executable file */
            exit(-1);
        }
    }
    else
    {
        /* TODO */
        /* Is child process successfully executed? */

        handle->u.process.state = QV_PROCESS_STATE_RUNNING;
        process->pid = (qv_pid_t)(ret);

        /* Dispatch a new thread to monitor the child */
        if (qv_threadpool_dispatch( \
                    threadpool, \
                    qv_process_thread_routine, \
                    handle, \
                    qv_process_thread_cont) != 0)
        { return -1; }
    }

    return 0;
}

static void qv_unixprocess_exec_task( \
        qv_handle_t *handle, void *data)
{
    qv_handle_t *handle_process = (qv_handle_t *)data;
    qv_unixprocess_exec(handle_process);
    qv_free(handle);
}

int qv_unixprocess_spawn( \
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

        if (qv_usertask_init( \
                    loop, defered_task, qv_unixprocess_exec_task, handle) != 0)
        {
            qv_free(defered_task);
            return -1;
        }
        
        return 0;
    }

    return qv_unixprocess_exec(handle);
}

int qv_unixprocess_kill( \
        qv_handle_t *handle, int signum)
{
    qv_process_t *process = &handle->u.process;

    return kill(process->pid, signum);
}

int qv_unixprocess_process( \
        qv_handle_t *handle)
{
    qv_unixprocess_exec(handle);

    return 0;
}

