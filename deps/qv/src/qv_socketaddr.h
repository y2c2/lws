/* qv : Socket Address
 * Copyright(c) 2016 y2c2 */

#ifndef QV_SOCKETADDR_H
#define QV_SOCKETADDR_H

#include "qv_config.h"
#include "qv_types.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#elif defined(QV_PLATFORM_NT)
#include <WS2tcpip.h>
#elif defined(QV_PLATFORM_MACOS)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

typedef enum
{
    QV_SOCKETADDR_TYPE_IPV4 = 4,
    QV_SOCKETADDR_TYPE_IPV6 = 6,
} qv_socketaddr_type_t;

struct qv_socketaddr
{
    qv_socketaddr_type_t type;
    union
    {
        union
        {
            qv_u8 as_u8[4];
            qv_u32 as_u32;
        } part_ipv4;
    } u;
    qv_port_t port;
};
typedef struct qv_socketaddr qv_socketaddr_t;

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
int qv_socketaddr_init_ipv4_sockaddr( \
        qv_socketaddr_t *qv_socketaddr, \
        struct sockaddr *sockaddr);

#endif

