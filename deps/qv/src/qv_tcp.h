/* qv : TCP
 * Copyright(c) 2016 y2c2 */

#ifndef QV_TCP_H
#define QV_TCP_H

#include "qv_handle.h"

int qv_tcp_init(qv_loop_t *loop, qv_handle_t *handle);
int qv_tcp_bind( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr);
int qv_tcp_listen( \
        qv_handle_t *handle, \
        int backlog, \
        qv_tcp_new_connection_cb_t new_connection_cb);
int qv_tcp_accept( \
        qv_handle_t *server, \
        qv_handle_t *client);
int qv_tcp_connect( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr, \
        qv_tcp_connected_cb_t cb);
int qv_tcp_recv_start( \
        qv_handle_t *handle, \
        qv_tcp_recv_cb_t recv_cb);
int qv_tcp_recv_stop(qv_handle_t *handle);
int qv_tcp_on_error( \
        qv_handle_t *handle, \
        qv_tcp_error_cb_t error_cb);
int qv_tcp_send( \
        qv_handle_t *handle, \
        const char *buf, \
        const qv_size_t nsize, \
        qv_tcp_send_cb_t send_cb);
int qv_tcp_close(qv_handle_t *handle);

int qv_tcp_process(qv_handle_t *handle, qv_u32 events);


#endif

