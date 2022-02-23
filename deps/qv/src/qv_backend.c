/* qv : Backend
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"
#ifdef QV_BACKEND_EPOLL
#include "qv_epoll.h"
#elif defined(QV_BACKEND_KQUEUE)
#include "qv_kqueue.h"
#elif defined(QV_BACKEND_SELECT)
#include "qv_select.h"
#elif defined(QV_BACKEND_IOCP)
#include "qv_iocp.h"
#endif
#include "qv_allocator.h"
#include "qv_watcher.h"
#include "qv_backend.h" 

int qv_backend_init(qv_backend_t *backend)
{
    int ret;

    if (qv_watcher_init(&backend->watcher) != 0)
    { return -1; }

#if defined(QV_BACKEND_EPOLL)
    if ((ret = qv_epoll_init(&backend->backend_handle)) != 0)
    { ret = -1; goto fail; }
#elif defined(QV_BACKEND_KQUEUE)
    if ((ret = qv_kqueue_init(&backend->backend_handle)) != 0)
    { ret = -1; goto fail; }
#elif defined(QV_BACKEND_SELECT)
    if ((ret = qv_select_init(&backend->backend_handle)) != 0)
    { ret = -1; goto fail; }
#elif defined(QV_BACKEND_IOCP)
	if ((ret = qv_iocp_init(&backend->backend_handle)) != 0)
	{ ret = -1; goto fail; }
#endif

    goto done;
fail:
    qv_backend_uninit(backend);
done:
    return ret;
}

void qv_backend_uninit(qv_backend_t *backend)
{
#if defined(QV_BACKEND_EPOLL)
    qv_epoll_uninit(&backend->backend_handle);
#elif defined(QV_BACKEND_KQUEUE)
    qv_kqueue_uninit(&backend->backend_handle);
#elif defined(QV_BACKEND_SELECT)
    qv_select_uninit(&backend->backend_handle);
#elif defined(QV_BACKEND_IOCP)
    qv_iocp_uninit(&backend->backend_handle);
#endif

    qv_watcher_uninit(&backend->watcher);
}

int qv_backend_ctl(qv_backend_t *backend, int fd, qv_backend_op_t op, qv_u32 triggered, void *data)
{
#if defined(QV_BACKEND_EPOLL)
    qv_epoll_op_t epoll_op = QV_EPOLL_OP_MOD;
    qv_u32 epoll_triggered = 0;

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            epoll_op = QV_EPOLL_OP_ADD;
            break;
        case QV_BACKEND_OP_DEL:
            epoll_op = QV_EPOLL_OP_DEL;
            break;
        case QV_BACKEND_OP_MOD:
            epoll_op = QV_EPOLL_OP_MOD;
            break;
    }

    if (triggered & QV_BACKEND_INPUT)
    { epoll_triggered |= QV_EPOLL_INPUT; }
    if (triggered & QV_BACKEND_OUTPUT)
    { epoll_triggered |= QV_EPOLL_OUTPUT; }

    if (qv_epoll_ctl( \
            backend->backend_handle, \
            fd, \
            epoll_op, epoll_triggered) != 0)
    { return -1; }

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            if (qv_watcher_set(&backend->watcher, fd, data) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_DEL:
            if (qv_watcher_clr(&backend->watcher, fd) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_MOD:
            break;
    }

    return 0;
#elif defined(QV_BACKEND_KQUEUE)
    qv_kqueue_op_t kqueue_op = QV_KQUEUE_OP_MOD;
    qv_u32 kqueue_triggered = 0;

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            kqueue_op = QV_KQUEUE_OP_ADD;
            break;
        case QV_BACKEND_OP_DEL:
            kqueue_op = QV_KQUEUE_OP_DEL;
            break;
        case QV_BACKEND_OP_MOD:
            kqueue_op = QV_KQUEUE_OP_MOD;
            break;
    }

    if (triggered & QV_BACKEND_INPUT)
    { kqueue_triggered |= QV_KQUEUE_INPUT; }
    if (triggered & QV_BACKEND_OUTPUT)
    { kqueue_triggered |= QV_KQUEUE_OUTPUT; }

    if (qv_kqueue_ctl( \
            backend->backend_handle, \
            fd, \
            kqueue_op, kqueue_triggered) != 0)
    { return -1; }

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            if (qv_watcher_set(&backend->watcher, fd, data) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_DEL:
            if (qv_watcher_clr(&backend->watcher, fd) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_MOD:
            break;
    }

    return 0;
#elif defined(QV_BACKEND_SELECT)
    qv_select_op_t select_op = QV_SELECT_OP_MOD;
    qv_u32 select_triggered = 0;

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            select_op = QV_SELECT_OP_ADD;
            break;
        case QV_BACKEND_OP_DEL:
            select_op = QV_SELECT_OP_DEL;
            break;
        case QV_BACKEND_OP_MOD:
            select_op = QV_SELECT_OP_MOD;
            break;
    }

    if (triggered & QV_BACKEND_INPUT)
    { select_triggered |= QV_SELECT_INPUT; }
    if (triggered & QV_BACKEND_OUTPUT)
    { select_triggered |= QV_SELECT_OUTPUT; }

    if (qv_select_ctl( \
            backend->backend_handle, \
            fd, \
            select_op, select_triggered) != 0)
    { return -1; }

    switch (op)
    {
        case QV_BACKEND_OP_ADD:
            if (qv_watcher_set(&backend->watcher, fd, data) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_DEL:
            if (qv_watcher_clr(&backend->watcher, fd) != 0)
            { return -1; }
            break;

        case QV_BACKEND_OP_MOD:
            break;
    }

    return 0;
#else
    return -1;
#endif
}

int qv_backend_wait(qv_backend_t *backend, int *fds, qv_u32 *events, int timeout)
{
#if defined(QV_BACKEND_EPOLL)
    return qv_epoll_wait(backend->backend_handle, fds, events, timeout);
#elif defined(QV_BACKEND_KQUEUE)
    return qv_kqueue_wait(backend->backend_handle, fds, events, timeout);
#elif defined(QV_BACKEND_SELECT)
    return qv_select_wait(backend->backend_handle, fds, events, timeout);
#elif defined(QV_BACKEND_IOCP)
    return qv_iocp_wait(backend->backend_handle, fds, events, timeout);
#endif
}

void *qv_backend_find(qv_backend_t *backend, int fd)
{
    return qv_watcher_get(&backend->watcher, fd);
}

int qv_backend_socket_tcp(void)
{
#if defined(QV_BACKEND_EPOLL)
    return qv_epoll_socket_tcp();
#elif defined(QV_BACKEND_KQUEUE)
    return qv_kqueue_socket_tcp();
#elif defined(QV_BACKEND_SELECT)
    return qv_select_socket_tcp();
#elif defined(QV_BACKEND_IOCP)
    return qv_iocp_socket_tcp();
#endif
}

int qv_backend_socket_udp(void)
{
#if defined(QV_BACKEND_EPOLL)
    return qv_epoll_socket_udp();
#elif defined(QV_BACKEND_KQUEUE)
    return qv_kqueue_socket_udp();
#elif defined(QV_BACKEND_SELECT)
    return qv_select_socket_udp();
#elif defined(QV_BACKEND_IOCP)
    return qv_iocp_socket_udp();
#endif
}

