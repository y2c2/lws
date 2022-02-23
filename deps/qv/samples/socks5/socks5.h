/* SOCKS5 Data Structure
 * Copyright(c) 2017 y2c2 */

#ifndef SOCKS5_H
#define SOCKS5_H

#include <qv.h>

typedef enum
{
    socks5_state_init = 0, /* Inbound <-> Server */
    socks5_state_ack = 1, /* Inbound <-> Server */
    socks5_state_established = 2, /* Inbound <-> Server */
    socks5_state_resolving = 3, /* Resolving the destination */
    socks5_state_connecting = 4, /* Server connecting to outbound */
    socks5_state_transfer = 5, /* Inbound <-> Server <-> Outbound */
    socks5_state_inbound_turningoff = 6, /* Inbound <-/-> Server <-> Outbound */
    socks5_state_outbound_turningoff = 7, /* Inbound <--> Server <-/-> Outbound */
    socks5_state_invalid = 8,
} socks5_state;

typedef struct
{
    int number;

    socks5_state state;
    qv_handle_t *inbound;
    qv_handle_t *outbound;
    qv_port_t port;
} socks5_ctx;

typedef struct
{
    char *bound_address;
    qv_port_t bound_port;
} socks5_startup;

/* Startup */
void socks5_startup_init(socks5_startup *startup);
int socks5_start(socks5_startup *startup);

/* Context */
void socks5_ctx_init(socks5_ctx *ctx);


#endif

