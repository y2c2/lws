/* qv : kqueue Backend
 * Copyright(c) 2016 y2c2 */

#ifndef QV_KQUEUE_H
#define QV_KQUEUE_H

#include "qv_types.h"

typedef enum
{
    QV_KQUEUE_OP_ADD = 0,
    QV_KQUEUE_OP_DEL,
    QV_KQUEUE_OP_MOD,
} qv_kqueue_op_t;

enum
{
    QV_KQUEUE_NONE = 0,
    QV_KQUEUE_INPUT = (1 << 0),
    QV_KQUEUE_OUTPUT = (1 << 1),

    /* Indicates Status */
    QV_KQUEUE_ERR = (1 << 2),
    QV_KQUEUE_HUP = (1 << 3)
};

int qv_kqueue_init(void **backend_handle);
int qv_kqueue_uninit(void **backend_handle);
int qv_kqueue_ctl(void *backend_handle, int fd, qv_kqueue_op_t op, qv_u32 triggered);
int qv_kqueue_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout);

int qv_kqueue_socket_tcp(void);
int qv_kqueue_socket_udp(void);

#endif

