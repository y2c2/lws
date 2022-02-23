/* qv : Epoll Backend
 * Copyright(c) 2016 y2c2 */

#ifndef QV_EPOLL_H
#define QV_EPOLL_H

#include "qv_types.h"

typedef enum
{
    QV_EPOLL_OP_ADD = 0,
    QV_EPOLL_OP_DEL,
    QV_EPOLL_OP_MOD,
} qv_epoll_op_t;

enum
{
    QV_EPOLL_NONE = 0,
    QV_EPOLL_INPUT = (1 << 0),
    QV_EPOLL_OUTPUT = (1 << 1),

    /* Indicates Status */
    QV_EPOLL_ERR = (1 << 2),
    QV_EPOLL_HUP = (1 << 3),
    QV_EPOLL_RDHUP = (1 << 4),
    QV_EPOLL_EPOLLET = (1 << 5)
};

int qv_epoll_init(void **backend_handle);
int qv_epoll_uninit(void **backend_handle);
int qv_epoll_ctl(void *backend_handle, int fd, qv_epoll_op_t op, qv_u32 triggered);
int qv_epoll_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout);

int qv_epoll_socket_tcp(void);
int qv_epoll_socket_udp(void);

#endif

