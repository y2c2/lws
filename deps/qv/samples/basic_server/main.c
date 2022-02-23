/* libqv demo 
 * Copyright(c) 2016 y2c2 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <qv.h>

#define DEFAULT_IP "0.0.0.0"
#define DEFAULT_PORT 9950
#define DEFAULT_BACKLOG 5

static qv_loop_t *g_loop;

/* #define DEBUG_TRACE_ON */

#ifdef DEBUG_TRACE_ON

#  define DEBUG_TRACE(_fmt, ...) \
    do { \
        fprintf(stdout, "%s:%d: info: " _fmt "\n", __FILE__, (int)__LINE__, __VA_ARGS__); \
        fflush(stdout); \
    } while (0)
#else
#  define DEBUG_TRACE(_fmt, ...) \
    do { } while (0)
#endif

void sighandler(int num)
{
    if (num == SIGINT) 
    {
        fprintf(stderr, "Control-C received, exit...\n");
        qv_loop_stop(g_loop);
    }
}

static void qv_tcp_send_cb( \
        qv_handle_t *handle, \
        int status)
{
    if (status < 0)
    {
        fprintf(stderr, "error: send error %d\n", status);
        qv_tcp_close(handle);
        free(handle);
        return;
    }
    else if (status == 0)
    {
        /* Everything is OK */
    }
}

static void qv_tcp_recv_cb( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf)
{
    static const char* response = "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: Closed\r\n"
        "\r\n"
        "Hello, World!";
    qv_size_t response_len = strlen(response);

    (void)buf;

    if (nread < 0)
    {
        DEBUG_TRACE("qv_tcp_recv_cb(nread=%d, fd=%d)", (int)nread, \
                handle->u.tcp.fd);
        qv_tcp_recv_stop(handle);
        qv_tcp_close(handle);
        free(handle);
        return;
    }
    else if (nread == 0)
    {
        DEBUG_TRACE("qv_tcp_recv_cb(nread=%d)", (int)nread);
        qv_tcp_recv_stop(handle);
        qv_tcp_close(handle);
        free(handle);
        return;
    }

    if (qv_tcp_send( \
                handle, \
                response, response_len, \
                qv_tcp_send_cb) != 0)
    {
        { fprintf(stderr, "error: send error\n"); return; }
        qv_tcp_recv_stop(handle);
        qv_tcp_close(handle);
        free(handle);
        return;
    }
}

static void qv_tcp_new_connection_cb( \
        qv_loop_t *loop, \
        qv_handle_t *server, \
        int status)
{
    qv_handle_t *client;

    /* Error */
    if (status < 0)
    { fprintf(stderr, "error: new connection error %d\n", status); return; }

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
        DEBUG_TRACE("qv_tcp_new_connection_cb(fd=%d)", client->u.tcp.fd);
        qv_tcp_recv_start(client, qv_tcp_recv_cb);
    }
    else
    {
        fprintf(stderr, "error: accept error\n");
        qv_tcp_close(client);
        free(client);
        return;
    }
}

int main(void)
{
    int ret = 0;
    qv_loop_t loop;
    qv_handle_t server;
    qv_socketaddr_t addr;

    /* Initialize memory management */
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

#if defined(QV_PLATFORM_NT)
    qv_winsock_init();
#endif

    /* Initialize event loop */
    if (qv_loop_init(&loop) != 0)
    {
        fprintf(stderr, "error: qv_loop_init() failed\n");
        goto fail;
    }
    qv_socketaddr_init_ipv4(&addr, DEFAULT_IP, DEFAULT_PORT);
    qv_tcp_init(&loop, &server);
    qv_tcp_bind(&server, &addr);
    if (qv_tcp_listen( \
                &server, \
                DEFAULT_BACKLOG, \
                qv_tcp_new_connection_cb) != 0)
    {
        fprintf(stderr, "error: listen failed\n");
        return -1;
    }

    printf("Listening on %s:%d\n", \
            DEFAULT_IP, DEFAULT_PORT);
    fflush(stdout);

    /* Signal */
    g_loop = &loop;
    signal(SIGINT, sighandler);

    /* Enter loop */
    qv_loop_run(&loop);
    qv_loop_close(&loop);

    goto done;
fail:
    ret = -1;
done:
#if defined(QV_PLATFORM_NT)
    qv_winsock_uninit();
#endif
    return ret;
}

