/* qv : Backend Select
 * Copyright(c) 2016 y2c2 */

#ifndef QV_SELECT_H
#define QV_SELECT_H

#include "qv_types.h"

typedef enum
{
    QV_SELECT_OP_ADD = 0,
    QV_SELECT_OP_DEL,
    QV_SELECT_OP_MOD,
} qv_select_op_t;

enum
{
    QV_SELECT_NONE = 0,
    QV_SELECT_INPUT = (1 << 0),
    QV_SELECT_OUTPUT = (1 << 1),

    /* Indicates Status */
    QV_SELECT_ERR = (1 << 2),
    QV_SELECT_HUP = (1 << 3)
};

int qv_select_init(void **backend_handle);
int qv_select_uninit(void **backend_handle);
int qv_select_ctl(void *backend_handle, int fd, qv_select_op_t op, qv_u32 triggered);
int qv_select_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout);

int qv_select_socket_tcp(void);
int qv_select_socket_udp(void);

#endif

