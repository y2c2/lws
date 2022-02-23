/* qv : Loop
 * Copyright(c) 2016 y2c2 */

#include <string.h>
#include "qv_config.h"
#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_backend.h"
#include "qv_timerqueue.h"
#include "qv_taskqueue.h"
#include "qv_tcp.h"
#include "qv_udp.h"
#include "qv_dns.h"
#include "qv_loop.h"
#include "qv_utils.h"

/* Loop */

int qv_loop_init(qv_loop_t *loop)
{
    int ret = 0;

    /* Clean */
    loop->stop = qv_false;
    loop->stashed_fd = -1;
    memset(&loop->backend, 0, sizeof(qv_backend_t));
    memset(&loop->threadpool, 0, sizeof(qv_threadpool_t));

    if ((loop->stashed_fd = qv_socket_tcp()) < 0)
    { ret = -1; goto fail; }
    if ((loop->event = qv_eventfd_new(&loop->event_trigger)) == QV_EVENTFD_INVALID)
    { ret = -1; goto fail; }
    if ((ret = qv_backend_init(&loop->backend)) != 0)
    { ret = -1; goto fail; }
    if ((ret = qv_threadpool_init( \
                    &loop->threadpool, \
                    loop->event_trigger, \
                    QV_WORKER_NUM)) != 0)
    { ret = -1; goto fail; }
    if ((ret = qv_timerqueue_init( \
                    &loop->timerqueue)) != 0)
    { ret = -1; goto fail; }
    if ((ret = qv_taskqueue_init( \
                    &loop->taskqueue)) != 0)
    { ret = -1; goto fail; }

    goto done;
fail:
done:
    return ret;
}

void qv_loop_close(qv_loop_t *loop)
{
    if (loop->event != QV_EVENTFD_INVALID)
    {
        if (loop->event == loop->event_trigger)
        {
            qv_eventfd_close(loop->event);
            loop->event = QV_EVENTFD_INVALID;
        }
        else
        {
            qv_eventfd_close(loop->event);
            loop->event = QV_EVENTFD_INVALID;
            qv_eventfd_close(loop->event_trigger);
            loop->event_trigger = QV_EVENTFD_INVALID;
        }
    }
    if (loop->stashed_fd != -1)
    {
        qv_closesocket(loop->stashed_fd);
        loop->stashed_fd = -1;
    }
    qv_threadpool_uninit(&loop->threadpool);
    qv_backend_uninit(&loop->backend);
    qv_timerqueue_uninit(&loop->timerqueue);
    qv_taskqueue_uninit(&loop->taskqueue);
}

static int qv_backend_timeout(qv_loop_t *loop)
{
    /* Pending task */
    if (!qv_taskqueue_empty(&loop->taskqueue)) return 0;

    /* Nearest timeout */
    return qv_timerqueue_next_timeout(&loop->timerqueue);
}

int qv_loop_run(qv_loop_t *loop)
{
    int *fds = NULL;
    qv_u32 *events = NULL;
    int nfds;
    int i;
    qv_handle_t *target_handle;
    int timeout;

    int ret = 0;

    if ((fds = (int *)qv_malloc( \
                    sizeof(int) * QV_WATCHER_MAXEVENTS)) == NULL)
    { return -1; }
    if ((events = (qv_u32 *)qv_malloc( \
                    sizeof(qv_u32) * QV_WATCHER_MAXEVENTS)) == NULL)
    { qv_free(fds); return -1; }

    if (qv_backend_ctl(&loop->backend, \
            loop->event, \
            QV_BACKEND_OP_ADD, \
            QV_BACKEND_INPUT, NULL) != 0)
    {
        qv_free(events);
        qv_free(fds);
        return -1;
    }

    loop->activated_fds = fds;
    for (;;)
    {
        if (loop->stop == qv_true) break;

        /* Backend Events */
        timeout = qv_backend_timeout(loop);

        if (timeout < 1000) timeout = 1000;

        nfds = qv_backend_wait(&loop->backend, fds, events, timeout);
        loop->activated_nfds = nfds;
        if (nfds < 0) continue;
        for (i = 0; i != nfds; i++)
        {
            if (fds[i] == -1) continue;
            if (fds[i] == loop->event)
            { qv_u64 v; qv_eventfd_read(loop->event, &v); continue; }

            loop->activated_idx = i;
            target_handle = qv_backend_find(&loop->backend, fds[i]);
            if (target_handle == NULL) { continue; }
            if (target_handle->type == QV_HANDLE_TYPE_TCP)
            { qv_tcp_process(target_handle, events[i]); }
            else if (target_handle->type == QV_HANDLE_TYPE_UDP)
            { qv_udp_process(target_handle, events[i]); }
        }

        /* Thread Pool Events */
        qv_threadpool_retrieve(&loop->threadpool);

        /* Pending User Tasks */
        qv_taskqueue_run(&loop->taskqueue);

        /* Timer Events */
        qv_timerqueue_run(&loop->timerqueue);
    }

    if (fds != NULL) qv_free(fds);
    if (events != NULL) qv_free(events);

    return ret;
}

int qv_loop_stop(qv_loop_t *loop)
{
    loop->stop = qv_true;

    return 0;
}

qv_backend_t *qv_loop_backend(qv_loop_t *loop)
{
    return &loop->backend;
}

qv_threadpool_t *qv_loop_threadpool(qv_loop_t *loop)
{
    return &loop->threadpool;
}

qv_taskqueue_t *qv_loop_taskqueue(qv_loop_t *loop)
{
    return &loop->taskqueue;
}

