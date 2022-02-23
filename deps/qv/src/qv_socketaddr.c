/* qv : Socket Address
 * Copyright(c) 2016 y2c2 */

#include "qv_socketaddr.h"

#define ISDIGIT(ch) (('0' <= (ch)) && ((ch) <= '9'))

#define MATCHDIGIT(dest) \
    do { \
        digit = 0; \
        if ((*p == '\0') || (!ISDIGIT(*p))) { return -1; } \
        do \
        { \
            digit = digit * 10 + ((qv_u32)(*p) - (qv_u32)('0')); \
            p++; \
        } while ((*p != '\0') && (ISDIGIT(*p))); \
        if (digit > 255) return -1; \
        (dest) = (qv_u8)digit; \
    } while (0)

#define MATCHDOT() \
    do { \
        if ((*p == '\0') || (*p != '.')) { return -1; } \
        p++; \
    } while (0)

static int qv_parse_ipv4(qv_u8 *u8_digits, const char *s)
{
    qv_u32 digit;
    const char *p = s;

    MATCHDIGIT(u8_digits[0]);
    MATCHDOT();
    MATCHDIGIT(u8_digits[1]);
    MATCHDOT();
    MATCHDIGIT(u8_digits[2]);
    MATCHDOT();
    MATCHDIGIT(u8_digits[3]);

    return 0;
}

int qv_socketaddr_init_ipv4( \
        qv_socketaddr_t *qv_socketaddr, \
        const char *s, \
        qv_port_t port)
{
    qv_socketaddr->type = QV_SOCKETADDR_TYPE_IPV4;
    qv_socketaddr->port = port;
    if (qv_parse_ipv4(qv_socketaddr->u.part_ipv4.as_u8, s) != 0)
    { return -1; }
    return 0;
}

int qv_socketaddr_init_ipv4_digits( \
        qv_socketaddr_t *qv_socketaddr, \
        const char *d, \
        qv_port_t port)
{
    qv_socketaddr->type = QV_SOCKETADDR_TYPE_IPV4;
    qv_socketaddr->port = port;
    qv_socketaddr->u.part_ipv4.as_u8[0] = (qv_u8)d[0];
    qv_socketaddr->u.part_ipv4.as_u8[1] = (qv_u8)d[1];
    qv_socketaddr->u.part_ipv4.as_u8[2] = (qv_u8)d[2];
    qv_socketaddr->u.part_ipv4.as_u8[3] = (qv_u8)d[3];

    return 0;
}

int qv_socketaddr_init_ipv4_addrinfo( \
        qv_socketaddr_t *qv_socketaddr, \
        struct addrinfo *addrinfo)
{
    struct sockaddr_in *addr = (struct sockaddr_in *)addrinfo->ai_addr;
    qv_u8 *src_p = (qv_u8 *)(&addr->sin_addr.s_addr);

    qv_socketaddr->type = QV_SOCKETADDR_TYPE_IPV4;
    qv_socketaddr->port = addr->sin_port;
    qv_socketaddr->u.part_ipv4.as_u8[0] = src_p[0];
    qv_socketaddr->u.part_ipv4.as_u8[1] = src_p[1];
    qv_socketaddr->u.part_ipv4.as_u8[2] = src_p[2];
    qv_socketaddr->u.part_ipv4.as_u8[3] = src_p[3];

    return 0;
}

int qv_socketaddr_init_ipv4_sockaddr( \
        qv_socketaddr_t *qv_socketaddr, \
        struct sockaddr *sockaddr)
{
    qv_u8 *src_p = (qv_u8 *)(&((struct sockaddr_in *)sockaddr)->sin_addr.s_addr);

    qv_socketaddr->type = QV_SOCKETADDR_TYPE_IPV4;
    qv_socketaddr->port = ((struct sockaddr_in *)sockaddr)->sin_port;
    qv_socketaddr->u.part_ipv4.as_u8[0] = src_p[0];
    qv_socketaddr->u.part_ipv4.as_u8[1] = src_p[1];
    qv_socketaddr->u.part_ipv4.as_u8[2] = src_p[2];
    qv_socketaddr->u.part_ipv4.as_u8[3] = src_p[3];

    return 0;
}

