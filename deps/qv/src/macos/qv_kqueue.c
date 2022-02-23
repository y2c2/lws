/* qv : kqueue Backend
 * Copyright(c) 2016 y2c2 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "qv_config.h"
#include "qv_allocator.h"
#include "qv_kqueue.h"


#define QV_KQUEUE_MAXEVENTS (QV_WATCHER_MAXEVENTS)

struct qv_kqueue
{
    int kqueue_fd;
    struct kevent *qv_events;
};
typedef struct qv_kqueue qv_kqueue_t;

int qv_kqueue_init(void **backend_handle)
{
    int ret = 0;
    qv_kqueue_t *new_kqueue_handle = NULL;

    if ((new_kqueue_handle = (qv_kqueue_t *)qv_malloc(sizeof(qv_kqueue_t))) == NULL)
    { return -1; }
    new_kqueue_handle->kqueue_fd = -1;
    new_kqueue_handle->qv_events = NULL;

    if ((new_kqueue_handle->kqueue_fd = kqueue()) == -1)
    { ret = -1; goto fail; }
    new_kqueue_handle->qv_events = (struct kevent *)qv_malloc( \
            sizeof(struct kevent) * QV_KQUEUE_MAXEVENTS);
    if (new_kqueue_handle->qv_events == NULL)
    { ret = -1; goto fail; }

    *backend_handle = new_kqueue_handle;

    goto done;
fail:
    qv_kqueue_uninit((void **)&new_kqueue_handle);
done:
    return ret;
}

int qv_kqueue_uninit(void **backend_handle)
{
    qv_kqueue_t *kqueue_handle = (qv_kqueue_t *)(*backend_handle);

    if (kqueue_handle->kqueue_fd != -1)
    {
        close(kqueue_handle->kqueue_fd);
    }
    if (kqueue_handle->qv_events != NULL)
    {
        qv_free(kqueue_handle->qv_events);
    }
    qv_free(kqueue_handle);

    return 0;
}

int qv_kqueue_ctl(void *backend_handle, int fd, qv_kqueue_op_t op, qv_u32 triggered)
{
    qv_kqueue_t *kqueue_handle = (qv_kqueue_t *)backend_handle;
    struct kevent event;
    u_int fflags = 0;

    if (triggered & QV_KQUEUE_INPUT) { fflags |= EVFILT_READ; }
    if (triggered & QV_KQUEUE_OUTPUT) { fflags |= EVFILT_WRITE; }

    switch (op)
    {
        case QV_KQUEUE_OP_ADD:
            EV_SET(&event, fd, fflags, EV_ADD, 0, 0, NULL);
            kevent(kqueue_handle->kqueue_fd, &event, 1, NULL, 0, NULL);
            break;
        case QV_KQUEUE_OP_DEL:
            EV_SET(&event, fd, fflags, EV_DELETE, 0, 0, NULL);
            kevent(kqueue_handle->kqueue_fd, &event, 1, NULL, 0, NULL);
            break;
        case QV_KQUEUE_OP_MOD:
            EV_SET(&event, fd, fflags, EV_DELETE, 0, 0, NULL);
            kevent(kqueue_handle->kqueue_fd, &event, 1, NULL, 0, NULL);
            EV_SET(&event, fd, fflags, EV_ADD, 0, 0, NULL);
            kevent(kqueue_handle->kqueue_fd, &event, 1, NULL, 0, NULL);
            break;
    }

    return 0;
}

int qv_kqueue_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout)
{
    qv_kqueue_t *kqueue_handle = (qv_kqueue_t *)backend_handle;
    int nev;
    int i;
    struct timespec ts;

    /* Set timeout */
    ts.tv_sec = 0;
    ts.tv_nsec = timeout * 1000;

    if ((nev = kevent( \
                    kqueue_handle->kqueue_fd, \
                    NULL, 0, kqueue_handle->qv_events, \
                    QV_KQUEUE_MAXEVENTS, \
                    &ts)) == -1)
    { return -1; }

    for (i = 0; i != nev; i++)
    {
        fds[i] = kqueue_handle->qv_events[i].ident;
        events[i] = 0;
        if (kqueue_handle->qv_events[i].fflags & EVFILT_READ) events[i] |= QV_KQUEUE_INPUT;
        if (kqueue_handle->qv_events[i].fflags & EVFILT_WRITE) events[i] |= QV_KQUEUE_OUTPUT;
        if (kqueue_handle->qv_events[i].flags & EV_ERROR) events[i] |= QV_KQUEUE_ERR;
    }

    return nev;
}

int qv_kqueue_socket_tcp(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int qv_kqueue_socket_udp(void)
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

