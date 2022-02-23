/* qv
 * Copyright(c) 2016-2018 y2c2 */

#ifndef QV_H
#define QV_H

#include "qv_config.h"
#include "qv_types.h"
#include "qv_socketaddr.h"
#include "qv_handle.h"
#include "qv_loop.h"

typedef void *(*qv_malloc_cb_t)(qv_size_t size);
typedef void (*qv_free_cb_t)(void *ptr);

#if defined(QV_PLATFORM_NT)
int qv_winsock_init(void);
int qv_winsock_uninit(void);
#endif

/* Memory Management */
void qv_allocator_set_malloc(qv_malloc_cb_t cb);
void qv_allocator_set_free(qv_free_cb_t cb);
void *qv_malloc(qv_size_t size);
void qv_free(void *ptr);

/* Loop */
int qv_loop_init(qv_loop_t *loop);
void qv_loop_close(qv_loop_t *loop);
int qv_loop_run(qv_loop_t *loop);
int qv_loop_stop(qv_loop_t *loop);

/* Address */
int qv_socketaddr_init_ipv4( \
        qv_socketaddr_t *qv_socketaddr, \
        const char *s, \
        qv_port_t port);
int qv_socketaddr_init_ipv4_digits( \
        qv_socketaddr_t *qv_socketaddr, \
        const char *d, \
        qv_port_t port);
int qv_socketaddr_init_ipv4_addrinfo( \
        qv_socketaddr_t *qv_socketaddr, \
        struct addrinfo *addrinfo);

/* Handle */
void qv_handle_set_data(qv_handle_t *handle, void *data);
void *qv_handle_get_data(qv_handle_t *handle);
void qv_handle_set_data_dtor(struct qv_handle *handle, qv_handle_data_dtor_t data_dtor);

/* TCP */
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
int qv_tcp_send( \
        qv_handle_t *handle, \
        const char *buf, \
        const qv_ssize_t nsize, \
        qv_tcp_send_cb_t send_cb);
int qv_tcp_close( \
        qv_handle_t *handle);

/* UDP */
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
int qv_tcp_on_error(qv_handle_t *handle, qv_tcp_error_cb_t error_cb);
int qv_udp_close(qv_handle_t *handle);

/* DNS */
int qv_getaddrinfo( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_getaddrinfo_cb getaddrinfo_cb, \
        const char *node, \
        const char *service, \
        const struct addrinfo* hints);
int qv_getnameinfo( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_getnameinfo_cb getnameinfo_cb, \
        const struct sockaddr *addr, \
        int flags);

/* Timer */
int qv_timer_init(qv_loop_t *loop, qv_handle_t *handle);
int qv_timer_set(qv_handle_t *handle, \
        qv_timer_cb_t cb, \
        qv_interval_t after, \
        qv_interval_t repeat);
int qv_timer_start(qv_handle_t *handle);
int qv_timer_stop(qv_handle_t *handle);
int qv_timer_again(qv_handle_t *handle);

/* Process */
int qv_process_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options);
int qv_process_kill( \
        qv_handle_t *handle, int signum);

/* User task */
int qv_usertask_init( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_usertask_cb_t cb, \
        qv_handle_t *data);

#endif

