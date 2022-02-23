/* qv : Backend
 * Copyright(c) 2016 y2c2 */

#ifndef QV_BACKEND_H
#define QV_BACKEND_H

#include "qv_watcher.h"

typedef enum
{
    QV_BACKEND_OP_ADD = 0,
    QV_BACKEND_OP_DEL,
    QV_BACKEND_OP_MOD,
} qv_backend_op_t;

enum
{
    QV_BACKEND_NONE = 0U,
    QV_BACKEND_INPUT = (1U << 0),
    QV_BACKEND_OUTPUT = (1U << 1),

    QV_BACKEND_ERR = (1U << 2),
    QV_BACKEND_HUP = (1U << 3),
    QV_BACKEND_RDHUP = (1U << 4),
    QV_BACKEND_ET = (1U << 5)
};

struct qv_backend
{
    qv_watcher_t watcher;
    void *backend_handle;
};
typedef struct qv_backend qv_backend_t;

/* Callbacks */
typedef int (*qv_backend_init_cb_t)(void **backend_handle);
typedef int (*qv_backend_uninit_cb_t)(void **backend_handle);
typedef int (*qv_backend_wait_cb_t)(void **backend_handle);

int qv_backend_init(qv_backend_t *backend);
void qv_backend_uninit(qv_backend_t *backend);
int qv_backend_ctl( \
        qv_backend_t *backend, \
        int fd, \
        qv_backend_op_t op, \
        qv_u32 triggered, \
        void *data);
int qv_backend_wait(qv_backend_t *backend, int *fds, qv_u32 *events, int timeout);
void *qv_backend_find(qv_backend_t *backend, int fd);

int qv_backend_socket_tcp(void);
int qv_backend_socket_udp(void);


#endif

