/* qv : UDP
 * Copyright(c) 2016 y2c2 */

#ifndef QV_UDP_H
#define QV_UDP_H

#include "qv_handle.h"

int qv_udp_init(qv_loop_t *loop, qv_handle_t *handle);
int qv_udp_bind( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr, \
        qv_u32 flags);
int qv_udp_getsockname(const qv_handle_t *handle, struct sockaddr *name, int *namelen);
int qv_udp_set_membership(qv_handle_t *handle, const char *multicast_addr, const char *interface_addr, qv_membership_t membership);
int qv_udp_set_multicast_loop(qv_handle_t *handle, qv_bool on);
int qv_udp_set_multicast_ttl(qv_handle_t *handle, int ttl);
int qv_udp_set_multicast_interface(qv_handle_t *handle, const char* interface_addr);
int qv_udp_set_broadcast(qv_handle_t *handle, qv_bool on);
int qv_udp_set_ttl(qv_handle_t *handle, int ttl);
int qv_udp_recv_start(qv_handle_t *handle, qv_udp_recv_cb_t recv_cb);
int qv_udp_recv_stop(qv_handle_t *handle);
int qv_udp_close(qv_handle_t *handle);

int qv_udp_process(qv_handle_t *handle, qv_u32 events);

#endif

