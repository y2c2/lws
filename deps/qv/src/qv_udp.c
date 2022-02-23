/* qv : UDP
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"

#include <errno.h>
#include <string.h>
#include <signal.h>

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined(QV_PLATFORM_NT)
#include <Winsock2.h>
#endif

#include "pqueue.h"

#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_udp.h"
#include "qv_loop.h"
#include "qv_backend.h"
#include "qv_libc.h"
#include "qv_utils.h"


int qv_udp_init(qv_loop_t *loop, qv_handle_t *handle)
{
    handle->type = QV_HANDLE_TYPE_UDP;
    handle->mode = QV_BACKEND_NONE;
    handle->loop = loop;
    qv_memset(&handle->u.udp, 0, sizeof(qv_udp_t));
    handle->u.udp.type = QV_UDP_TYPE_UNKNOWN;
    handle->u.udp.fd = -1;
    handle->u.udp.data.part_established.packet_buffer = NULL;
    handle->data = NULL;
    handle->data_dtor = NULL;

    return 0;
}

int qv_udp_bind( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr, \
        qv_u32 flags)
{
    struct sockaddr_in serveraddr;
    qv_u8 *dst_p;

    (void)flags;

    /* Fill address */
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons((qv_u16)addr->port);
    dst_p = (qv_u8 *)(&serveraddr.sin_addr.s_addr);
    dst_p[0] = addr->u.part_ipv4.as_u8[0];
    dst_p[1] = addr->u.part_ipv4.as_u8[1];
    dst_p[2] = addr->u.part_ipv4.as_u8[2];
    dst_p[3] = addr->u.part_ipv4.as_u8[3];

    /* Create socket */
    if ((handle->u.udp.fd = qv_backend_socket_udp()) < 0)
    { return -1; }
    /* Defaultly reuse */
    qv_reuse(handle->u.udp.fd, qv_true);
    qv_blocking(handle->u.udp.fd, qv_false);

    if ((handle->u.udp.data.part_established.packet_buffer = pqueue_new( \
                    qv_malloc, qv_free)) == NULL)
    { return -1; }

    /* Bind address */
    if (bind(handle->u.udp.fd, \
            (struct sockaddr *)&serveraddr, \
            sizeof(serveraddr)) != 0)
    { return -1; }
    memcpy(&handle->u.udp.addr, addr, sizeof(qv_socketaddr_t));
    /* Bound */
    handle->u.udp.type = QV_UDP_TYPE_ESTABLISHED;

    return 0;
}

int qv_udp_getsockname(const qv_handle_t *handle, struct sockaddr *name, int *namelen)
{
    socklen_t addrlen;

    if (getsockname(handle->u.udp.fd, name, &addrlen) != 0)
    { return -1; }

    *namelen = (int)addrlen;

    return 0;
}

int qv_udp_set_membership(qv_handle_t *handle, const char *multicast_addr, const char *interface_addr, qv_membership_t membership)
{
    struct ip_mreq group;

    if ((group.imr_multiaddr.s_addr = inet_addr(multicast_addr)) == INADDR_NONE)
    { return -1; }
    if ((group.imr_interface.s_addr = inet_addr(interface_addr)) == INADDR_NONE)
    { return -1; }

    switch (membership)
    {
        case QV_MEMBERSHIP_JOIN:
            if (setsockopt(handle->u.udp.fd, \
                        IPPROTO_IP, \
                        IP_ADD_MEMBERSHIP, \
                        (char *)&group, sizeof(group)) < 0)
            { return -1; }
            break;

        case QV_MEMBERSHIP_LEAVE:
            if (setsockopt(handle->u.udp.fd, \
                        IPPROTO_IP, \
                        IP_DROP_MEMBERSHIP, \
                        (char *)&group, sizeof(group)) < 0)
            { return -1; }
            break;
    }

    return 0;
}

int qv_udp_set_multicast_loop(qv_handle_t *handle, qv_bool on)
{
    char loopch = (on != qv_false) ? 1 : 0;

    if (setsockopt(handle->u.udp.fd, \
                IPPROTO_IP, \
                IP_MULTICAST_LOOP, \
                (char *)&loopch, sizeof(loopch)) < 0)
    { return -1; }

    return 0;
}

int qv_udp_set_multicast_ttl(qv_handle_t *handle, int ttl)
{
    char ttlch = (char)ttl;

    if (setsockopt(handle->u.udp.fd, \
                IPPROTO_IP, \
                IP_MULTICAST_TTL, \
                (char *)&ttlch, sizeof(ttlch)) < 0)
    { return -1; }

    return 0;
}

int qv_udp_set_multicast_interface(qv_handle_t *handle, const char* interface_addr)
{
    struct in_addr localinterface;

    localinterface.s_addr = inet_addr(interface_addr);

    if (setsockopt(handle->u.udp.fd, \
                IPPROTO_IP, \
                IP_MULTICAST_IF, \
                (char *)&localinterface, \
                sizeof(localinterface)) < 0)
    { return -1; }

    return 0;
}

int qv_udp_set_broadcast(qv_handle_t *handle, qv_bool on)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    int broadcastenabled = (on != qv_false) ? 1 : 0;
#elif defined(QV_PLATFORM_NT)
    char broadcastenabled = (on != qv_false) ? 1 : 0;
#endif

    if (setsockopt(handle->u.udp.fd, \
                SOL_SOCKET, \
                SO_BROADCAST, \
                &broadcastenabled, 
                sizeof(broadcastenabled)) < 0)
    { return -1; }

    return 0;
}

int qv_udp_set_ttl(qv_handle_t *handle, int ttl)
{
    char ttlch = (char)ttl;

    if (setsockopt(handle->u.udp.fd, \
                IPPROTO_IP, \
                IP_TTL, \
                (char *)&ttlch, sizeof(ttlch)) < 0)
    { return -1; }

    return 0;
}

int qv_udp_recv_start(qv_handle_t *handle, qv_udp_recv_cb_t recv_cb)
{
    char *data;
    qv_size_t len;
    pqueue_t *packet_buffer = handle->u.udp.data.part_established.packet_buffer;
    qv_socketaddr_t addr;

    /* Set callback */
    handle->u.udp.callbacks.part_established.recv_cb = recv_cb;

    /* Process pending data */
    while (pqueue_count(packet_buffer) != 0)
    {
        pqueue_pop(packet_buffer, &data, &len, &addr);

        recv_cb(handle, (qv_ssize_t)len, (char *)data, &addr);

        qv_free(data);
    }

    return 0;
}

int qv_udp_recv_stop(qv_handle_t *handle)
{
    /* Clear callback */
    handle->u.udp.callbacks.part_established.recv_cb = NULL;

    return 0;
}

int qv_udp_close(qv_handle_t *handle)
{
    if (handle->u.udp.data.part_established.packet_buffer != NULL)
    {
        pqueue_destroy(handle->u.udp.data.part_established.packet_buffer);
    }

    qv_backend_ctl( \
            &handle->loop->backend, \
            handle->u.udp.fd, \
            QV_BACKEND_OP_DEL, \
            0, \
            NULL);

    qv_closesocket(handle->u.udp.fd);

    return 0;
}

static int qv_udp_process_established_recv(qv_handle_t *handle)
{
    qv_ssize_t nread;
    char buffer[QV_RECV_BUFFER_SIZE];
    pqueue_t *packet_buffer = handle->u.udp.data.part_established.packet_buffer;
    struct sockaddr src_addr;
    qv_socketaddr_t addr;
    int fd = handle->u.udp.fd;
    socklen_t socklen = sizeof(struct sockaddr);
    char *data;
    qv_size_t len;
    qv_udp_recv_cb_t recv_cb = handle->u.udp.callbacks.part_established.recv_cb;

    for (;;)
    {
        nread = recvfrom(fd, \
                buffer, QV_RECV_BUFFER_SIZE, \
                0, &src_addr, &socklen);
        if (nread <= 0) break;

        qv_socketaddr_init_ipv4_sockaddr(&addr, &src_addr);

        if (pqueue_push(packet_buffer, buffer, (qv_size_t)nread, &addr) != 0)
        { /* ignore */ }
    }

    /* Process pending data */
    while (pqueue_count(packet_buffer) != 0)
    {
        pqueue_pop(packet_buffer, &data, &len, &addr);

        recv_cb(handle, (qv_ssize_t)len, (char *)data, &addr);

        qv_free(data);
    }

    return 0;
}

int qv_udp_process(qv_handle_t *handle, qv_u32 events)
{
    switch (handle->u.udp.type)
    {
        case QV_UDP_TYPE_UNKNOWN:
            break;

        case QV_UDP_TYPE_ESTABLISHED:
            if (events & QV_BACKEND_INPUT)
            { qv_udp_process_established_recv(handle); }
            break;
    }

    return 0;
}

