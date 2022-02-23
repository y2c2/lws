/* qv : DNS
 * Copyright(c) 2016 y2c2 */

#ifndef QV_DNS_H
#define QV_DNS_H

#include "qv_handle.h"

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

int qv_getaddrinfo_close(qv_handle_t *handle);

int qv_getaddrinfo_process(qv_handle_t *handle);

int qv_getnameinfo_process(qv_handle_t *handle);

#endif

