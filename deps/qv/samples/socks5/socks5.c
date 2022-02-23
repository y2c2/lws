/* SOCKS5 Data Structure
 * Copyright(c) 2017 y2c2 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "socks5.h"

#define DEFAULT_BACKLOG 5

qv_loop_t *g_loop;


/* Declarations 
 *
 * Interfaces listed by the order of a typical conversation */

static void server_on_connection(qv_loop_t *loop, qv_handle_t *server, int status);
static void inbound_generic_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void inbound_init_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void inbound_ack_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void inbound_ack_on_sent_acknowledge(qv_handle_t *inbound, int status);
static void inbound_established_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void inbound_resolving_on_resolved(qv_handle_t *resolver, int status, struct addrinfo* res);
static void outbound_on_connected(qv_loop_t *loop, qv_handle_t *outbound, int status);
static void inbound_on_connect_outbound_fail_sent(qv_handle_t *inbound, int status);
static void inbound_on_connect_outbound_success_sent(qv_handle_t *inbound, int status);
static void inbound_connecting_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void inbound_transfer_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void outbound_transfer_on_sent(qv_handle_t *outbound, int status);
static void outbound_transfer_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);
static void outbound_transfer_on_sent(qv_handle_t *outbound, int status);
static void outbound_transfer_on_error(qv_handle_t *outbound, int status);
static void inbound_transfer_on_error(qv_handle_t *outbound, int status);
static void inbound_invalid_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf);

/* Implementations */

#define CLOSE_HANDLE(handle) \
    do { if ((handle) == NULL) break; qv_tcp_recv_stop(handle); qv_tcp_close(handle); free(handle); handle = NULL; } while (0)

#define CLOSE_CTX(ctx) \
    do { CLOSE_HANDLE(ctx->inbound); CLOSE_HANDLE(ctx->outbound); free(ctx); } while (0)

static qv_bool socks5_method_supported( \
        const int nmethods, const char *methods)
{
    int i;
    for (i = 0; i < nmethods; i++)
    { if (methods[i] == 0x00) return qv_true; }
    return qv_false;
}

static void inbound_init_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_init_on_data(nread=%d)\n", (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    if (!(nread >= 3 && buf[0] == 0x05 && \
                socks5_method_supported((int)buf[1], buf + 2) == qv_true))
    { CLOSE_CTX(ctx); return; }
    qv_tcp_send(inbound, "\x05\x00", 2, inbound_ack_on_sent_acknowledge);

    ctx->state = socks5_state_ack;
}

static void inbound_ack_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)buf;

    printf("inbound_ack_on_data(nread=%d)\n", (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    CLOSE_CTX(ctx);
}

static void inbound_ack_on_sent_acknowledge(qv_handle_t *inbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_ack_on_sent_acknowledge(status=%d)\n", status);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    if (status < 0) { CLOSE_CTX(ctx); return; }
    else { ctx->state = socks5_state_established; }
}

static void inbound_established_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_established_on_data(nread=%d)\n", (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    if (nread < 3) { CLOSE_CTX(ctx); return; }

    if (buf[1] == 0x01)
    {
        qv_u8 atype;

        /* CONNECT */

        if (!(nread >= 4)) { CLOSE_CTX(ctx); return; }
        atype = buf[3];

        if (atype == 0x01)
        {
            qv_socketaddr_t addr;
            char *dst_addr = (char *)&buf[4];
            qv_u16 port = ((qv_u16)buf[8] << 8) | ((qv_u16)buf[9]);
            if ((ctx->outbound = (qv_handle_t *)malloc(sizeof(qv_handle_t))) == NULL)
            { CLOSE_CTX(ctx); return; }
            qv_tcp_init(inbound->loop, ctx->outbound);
            qv_handle_set_data(ctx->outbound, ctx);
            qv_socketaddr_init_ipv4_digits(&addr, dst_addr, port);
            if (qv_tcp_connect(ctx->outbound, &addr, outbound_on_connected) != 0)
            { CLOSE_CTX(ctx); return; }

            ctx->state = socks5_state_connecting;
        }
        else if (atype == 0x03)
        {
            struct addrinfo hints;
            char *addr = buf + 5;
            qv_u8 *port_p;
            qv_port_t port;
            qv_handle_t *resolver = NULL;
            qv_u8 domain_len = (qv_u8)buf[4];
            port_p = (qv_u8 *)buf + 5 + domain_len;
            port = ((qv_u16)(*port_p) << 8) | (qv_u16)(*(port_p + 1));
            ctx->port = port;

            hints.ai_family = PF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = 0;
            hints.ai_next = NULL;
            hints.ai_canonname = NULL;

            if ((resolver = (qv_handle_t *)malloc(sizeof(qv_handle_t))) == NULL)
            { CLOSE_CTX(ctx); return; }
            qv_handle_set_data(resolver, ctx);
            {
                char addr_buf[256];
                memcpy(addr_buf, addr, domain_len);
                addr_buf[domain_len] = '\0';
                if (qv_getaddrinfo(inbound->loop, \
                            resolver, \
                            inbound_resolving_on_resolved, addr_buf, "", &hints) != 0)
                { CLOSE_CTX(ctx); free(resolver); return; }
            }

            ctx->state = socks5_state_resolving;
        }
        else if (atype == 0x04) { CLOSE_CTX(ctx); return; }
        else { CLOSE_CTX(ctx); return; }
    }
    else if (buf[1] == 0x02) { CLOSE_CTX(ctx); return; }
    else if (buf[1] == 0x03) { CLOSE_CTX(ctx); return; }
    else { CLOSE_CTX(ctx); return; }
}

static void inbound_resolving_on_resolved(qv_handle_t *resolver, int status, struct addrinfo* res)
{
    socks5_ctx *ctx = qv_handle_get_data(resolver);
    qv_loop_t *loop = resolver->loop;
    qv_socketaddr_t addr;
    qv_handle_t *outbound;

    printf("inbound_resolving_on_resolved(ctx=%d,status=%d)\n", ctx->number, status);
    /* printf("watcher.used=%d\n", (int)ctx->inbound->loop->backend.watcher.used); */

    free(resolver); resolver = NULL;

    if (ctx->state == socks5_state_invalid)
    { freeaddrinfo(res); CLOSE_CTX(ctx); return; }

    if ((status < 0 ) || \
            (qv_socketaddr_init_ipv4_addrinfo(&addr, res) != 0))
    { freeaddrinfo(res); CLOSE_CTX(ctx); return; }

    addr.port = ctx->port;

    outbound = (qv_handle_t *)malloc(sizeof(qv_handle_t));
    ctx->outbound = outbound;
    qv_tcp_init(loop, outbound);
    qv_handle_set_data(ctx->outbound, ctx);
    if (qv_tcp_connect(outbound, &addr, outbound_on_connected) != 0)
    { freeaddrinfo(res); CLOSE_CTX(ctx); return; }

    freeaddrinfo(res);

    ctx->state = socks5_state_connecting;
}

static void outbound_on_connected( \
        qv_loop_t *loop, \
        qv_handle_t *outbound, \
        int status)
{
    socks5_ctx *ctx = qv_handle_get_data(outbound);

    (void)loop;

    printf("outbound_on_connected(status=%d)\n", status); fflush(stdout);
    printf("watcher.used=%d\n", (int)outbound->loop->backend.watcher.used);

    if (ctx->state == socks5_state_inbound_turningoff)
    {
        CLOSE_CTX(ctx);
        return;
    }
    ctx->outbound = outbound;

    if (status < 0)
    {
        qv_tcp_send(ctx->inbound, "\x05\x01", 2, inbound_on_connect_outbound_fail_sent);
        return;
    }

    qv_tcp_send(ctx->inbound, \
            "\x05\x00\x00" "\x01" "\x00\x00\x00\x00" "\x00\x00", 10,  \
            inbound_on_connect_outbound_success_sent);
}

static void inbound_resolving_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)nread;
    (void)buf;

    /* Should not receive any data while connecting,
     * but we could not just close it because in the connecting process */

    printf("inbound_resolving_on_data(nread=%d)\n", (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    CLOSE_HANDLE(ctx->inbound);

    ctx->state = socks5_state_invalid;
}

static void inbound_connecting_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)nread;
    (void)buf;

    /* Should not receive any data while connecting,
     * but we could not just close it because in the connecting process */

    printf("TODO: inbound_connecting_on_data(nread=%d)\n", (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    CLOSE_CTX(ctx);
}

static void inbound_transfer_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_transfer_on_data(ctx=%d,nread=%d)\n", ctx->number, (int)nread);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    if (nread < 0) { CLOSE_CTX(ctx); return; }

    if (qv_tcp_send(ctx->outbound, buf, nread, outbound_transfer_on_sent) < 0)
    { CLOSE_HANDLE(ctx->outbound); CLOSE_HANDLE(inbound); free(ctx); return; }
}

static void inbound_on_sent(qv_handle_t *inbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);


    if (ctx->state == socks5_state_transfer)
    {
        printf("inbound_transfer_on_sent(ctx=%d,status=%d)\n", ctx->number, status);
        printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);
        if (status < 0) CLOSE_CTX(ctx);
    }
    else if (ctx->state == socks5_state_outbound_turningoff)
    {
        printf("inbound_outbound_turningoff_on_sent(ctx=%d,status=%d)\n", ctx->number, status);
        printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);
        CLOSE_CTX(ctx);
    }
    else
    {
        printf("inbound_UNKNOWN_on_sent(ctx=%d,status=%d)\n", ctx->number, status);
        printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);
    }
}

static void outbound_transfer_on_sent(qv_handle_t *outbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(outbound);

    printf("outbound_transfer_on_sent(ctx=%d)\n", ctx->number); fflush(stdout);
    printf("watcher.used=%d\n", (int)outbound->loop->backend.watcher.used);

    if (status < 0) CLOSE_CTX(ctx);
}


static void outbound_transfer_on_data(qv_handle_t *outbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(outbound);

    if (ctx->state == socks5_state_transfer)
    {
        printf("outbound_transfer_on_data(ctx=%d,nread=%d)\n", \
                ctx->number, \
                (int)nread); fflush(stdout);
        printf("watcher.used=%d\n", (int)outbound->loop->backend.watcher.used);

        /* Though error caused on outbound side, some data is probably in
         * transfer process, we could not just turn off the inbound side. */
        if (nread < 0)
        {
            if (ctx->inbound->u.tcp.data.part_established.send_buffer.size != 0)
            {
                CLOSE_HANDLE(ctx->outbound);
                ctx->state = socks5_state_outbound_turningoff;
            }
            else
            {
                CLOSE_CTX(ctx);
            }
            return;
        }
    }
    else if (ctx->state == socks5_state_inbound_turningoff)
    {
        printf("outbound_inbound_turningoff_on_data(ctx=%d,nread=%d)\n", \
                ctx->number, \
                (int)nread); fflush(stdout);
    }
    else if (ctx->state == socks5_state_outbound_turningoff)
    {
        printf("outbound_outbound_turningoff_on_data(ctx=%d,nread=%d)\n", \
                ctx->number, \
                (int)nread); fflush(stdout);
    }
    else
    {
        printf("TODO: outbound_UNKNOWN_transfer_on_data\n");
    }
    printf("watcher.used=%d\n", (int)outbound->loop->backend.watcher.used);

    if (qv_tcp_send(ctx->inbound, buf, nread, inbound_on_sent) < 0) CLOSE_CTX(ctx);
}

static void outbound_transfer_on_error(qv_handle_t *outbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(outbound);

    (void)status;
    printf("outbound_transfer_on_error(ctx=%d)\n", ctx->number);
    printf("watcher.used=%d\n", (int)outbound->loop->backend.watcher.used);

    CLOSE_HANDLE(ctx->outbound);
    ctx->state = socks5_state_outbound_turningoff;
}

static void inbound_transfer_on_error(qv_handle_t *inbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)status;

    printf("inbound_transfer_on_error(ctx=%d)\n", ctx->number);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    /* ctx->state = socks5_state_inbound_turningoff; */
    CLOSE_CTX(ctx);
}

static void inbound_on_connect_outbound_success_sent(qv_handle_t *inbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_on_connect_outbound_success_sent()\n"); fflush(stdout);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    if (status < 0) CLOSE_CTX(ctx);

    qv_tcp_recv_start(ctx->outbound, outbound_transfer_on_data);
    qv_tcp_on_error(ctx->outbound, outbound_transfer_on_error);
    qv_tcp_on_error(ctx->inbound, inbound_transfer_on_error);

    ctx->state = socks5_state_transfer;
}

static void inbound_on_connect_outbound_fail_sent(qv_handle_t *inbound, int status)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    printf("inbound_on_connect_outbound_fail_sent()\n"); fflush(stdout);
    printf("watcher.used=%d\n", (int)inbound->loop->backend.watcher.used);

    (void)status;
    CLOSE_HANDLE(inbound);
    free(ctx);
}

static void inbound_outbound_turningoff_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)nread;
    (void)buf;

    CLOSE_CTX(ctx);
}

static void inbound_invalid_on_data(qv_handle_t *inbound, qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    (void)buf;
    (void)ctx;

    printf("inbound_invalid_on_data(nread=%d)\n", (int)nread); fflush(stdout);

    CLOSE_CTX(ctx);
}

static void inbound_generic_on_data( \
        qv_handle_t *inbound, \
        qv_ssize_t nread, char *buf)
{
    socks5_ctx *ctx = qv_handle_get_data(inbound);

    /*
    printf("inbound_generic_on_data(ctx=%d,fd=%d,nread=%d,state=%d)\n", \
            ctx->number, inbound->u.tcp.fd, (int)nread, (int)ctx->state);
    fflush(stdout);
    */

    switch (ctx->state)
    {
        case socks5_state_init:
            inbound_init_on_data(inbound, nread, buf);
            break;

        case socks5_state_ack:
            inbound_ack_on_data(inbound, nread, buf);
            break;

        case socks5_state_established:
            inbound_established_on_data(inbound, nread, buf);
            break;

        case socks5_state_resolving:
            inbound_resolving_on_data(inbound, nread, buf);
            break;

        case socks5_state_connecting:
            inbound_connecting_on_data(inbound, nread, buf);
            return;

        case socks5_state_transfer:
            inbound_transfer_on_data(inbound, nread, buf);
            return;

        case socks5_state_inbound_turningoff:
            printf("TODO: socks5_state_inbound_turningoff\n");
            break;

        case socks5_state_outbound_turningoff:
            inbound_outbound_turningoff_on_data(inbound, nread, buf);
            break;

        case socks5_state_invalid:
            inbound_invalid_on_data(inbound, nread, buf);
            break;
    }
}

static void server_on_connection(qv_loop_t *loop, qv_handle_t *server, int status)
{
    qv_handle_t *client;
    static int ctx_number = 0;

    printf("server_on_connection(status=%d)\n", status); fflush(stdout);

    if (status < 0)
    {
        fprintf(stderr, "error: new connection error (status=%d)\n", status);
        fflush(stderr);
        return;
    }

    client = (qv_handle_t *)malloc(sizeof(qv_handle_t));
    if (qv_tcp_init(loop, client) != 0)
    {
        fprintf(stderr, "error: initial socket error\n");
        fprintf(stderr, "%s\n", strerror(errno));
        free(client);
        return; 
    }
    if (qv_tcp_accept(server, client) == 0)
    {
        {
            socks5_ctx *ctx = (socks5_ctx *)malloc(sizeof(socks5_ctx));
            if (ctx == NULL) { CLOSE_HANDLE(client); return; }
            socks5_ctx_init(ctx);
            ctx->number = ctx_number++;
            ctx->inbound = client;
            qv_handle_set_data(client, ctx);
        }
        qv_tcp_recv_start(client, inbound_generic_on_data);
    }
    else
    { CLOSE_HANDLE(client); return; }
}

static int start_server(const char *host, const qv_port_t port)
{
    qv_loop_t loop;
    qv_handle_t server;
    qv_socketaddr_t addr;

    /* Initialize memory management */
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

    /* Initialize loop */
    if (qv_loop_init(&loop) != 0)
    {
        fprintf(stderr, "error: initialize loop failed\n");
        return -1;
    }

    qv_socketaddr_init_ipv4(&addr, host, port);
    qv_tcp_init(&loop, &server);
    qv_tcp_bind(&server, &addr);
    if (qv_tcp_listen(&server, DEFAULT_BACKLOG, server_on_connection) != 0)
    {
        fprintf(stderr, "error: listen failed\n");
        return -1;
    }

    printf("Listening on %s:%d\n", host, port);
    fflush(stdout);

    /* Signal */
    g_loop = &loop;

    /* Enter loop */
    qv_loop_run(&loop);
    qv_loop_close(&loop);

    return 0;
}

/* Startup */
void socks5_startup_init(socks5_startup *startup)
{
    startup->bound_address = NULL;
    startup->bound_port = 0;
}

int socks5_start(socks5_startup *startup)
{
    return start_server(startup->bound_address, startup->bound_port);
}

void socks5_ctx_init(socks5_ctx *ctx)
{
    ctx->state = socks5_state_init;
    ctx->inbound = NULL;
    ctx->outbound = NULL;
}

