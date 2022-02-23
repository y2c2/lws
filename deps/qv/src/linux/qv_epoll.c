/* qv : Epoll Backend
 * Copyright(c) 2016 y2c2 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "qv_config.h"
#include "qv_allocator.h"
#include "qv_epoll.h"


#define QV_EPOLL_MAXEVENTS (QV_WATCHER_MAXEVENTS)

struct qv_epoll
{
    int epoll_fd;
    struct epoll_event *qv_events;
};
typedef struct qv_epoll qv_epoll_t;

int qv_epoll_init(void **backend_handle)
{
    int ret = 0;
    qv_epoll_t *new_epoll_handle = NULL;

    if ((new_epoll_handle = (qv_epoll_t *)qv_malloc(sizeof(qv_epoll_t))) == NULL)
    { return -1; }
    new_epoll_handle->epoll_fd = -1;
    new_epoll_handle->qv_events = NULL;

    if ((new_epoll_handle->epoll_fd = epoll_create1(0)) == -1)
    { ret = -1; goto fail; }
    new_epoll_handle->qv_events = (struct epoll_event *)qv_malloc( \
            sizeof(struct epoll_event) * QV_EPOLL_MAXEVENTS);
    if (new_epoll_handle->qv_events == NULL)
    { ret = -1; goto fail; }

    *backend_handle = new_epoll_handle;

    goto done;
fail:
    qv_epoll_uninit((void **)&new_epoll_handle);
done:
    return ret;
}

int qv_epoll_uninit(void **backend_handle)
{
    qv_epoll_t *epoll_handle = (qv_epoll_t *)(*backend_handle);

    if (epoll_handle->epoll_fd != -1)
    {
        close(epoll_handle->epoll_fd);
    }
    if (epoll_handle->qv_events != NULL)
    {
        qv_free(epoll_handle->qv_events);
    }
    qv_free(epoll_handle);

    return 0;
}

int qv_epoll_ctl(void *backend_handle, int fd, qv_epoll_op_t op, qv_u32 triggered)
{
    qv_epoll_t *epoll_handle = (qv_epoll_t *)backend_handle;
    struct epoll_event event;

    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = fd;
    event.events = 0;

    if (triggered & QV_EPOLL_INPUT) { event.events |= EPOLLIN; }
    if (triggered & QV_EPOLL_OUTPUT) { event.events |= EPOLLOUT; }
    if (triggered & QV_EPOLL_RDHUP) { event.events |= EPOLLRDHUP; }
    if (triggered & QV_EPOLL_EPOLLET) { event.events |= EPOLLET; }

    switch (op)
    {
        case QV_EPOLL_OP_ADD:
            if (epoll_ctl(epoll_handle->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
            { return -1; }
            break;
        case QV_EPOLL_OP_DEL:
            if (epoll_ctl(epoll_handle->epoll_fd, EPOLL_CTL_DEL, fd, &event) == -1)
            { return -1; }
            break;
        case QV_EPOLL_OP_MOD:
            if (epoll_ctl(epoll_handle->epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1)
            { return -1; }
            break;
    }

    return 0;
}

int qv_epoll_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout)
{
    qv_epoll_t *epoll_handle = (qv_epoll_t *)backend_handle;
    int nfds;
    int i;

    memset(epoll_handle->qv_events, 0, QV_EPOLL_MAXEVENTS * sizeof(struct epoll_event));

    if ((nfds = epoll_wait( \
                    epoll_handle->epoll_fd, \
                    epoll_handle->qv_events, \
                    QV_EPOLL_MAXEVENTS, \
                    timeout)) == -1)
    { return -1; }

    for (i = 0; i != nfds; i++)
    {
        fds[i] = epoll_handle->qv_events[i].data.fd;
        events[i] = 0;
        if (epoll_handle->qv_events[i].events & EPOLLIN) events[i] |= QV_EPOLL_INPUT;
        if (epoll_handle->qv_events[i].events & EPOLLOUT) events[i] |= QV_EPOLL_OUTPUT;
        if (epoll_handle->qv_events[i].events & EPOLLERR) events[i] |= QV_EPOLL_ERR;
        if (epoll_handle->qv_events[i].events & EPOLLHUP) events[i] |= QV_EPOLL_HUP;
    }

    return nfds;
}

int qv_epoll_socket_tcp(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int qv_epoll_socket_udp(void)
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

