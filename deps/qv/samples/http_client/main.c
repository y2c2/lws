#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <qv.h>

#define BUFFER_SIZE 4096

static char *g_hostname;

void on_recv( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf)
{
    qv_loop_t *loop = handle->loop;

    if (nread < 0)
    {
        qv_tcp_recv_stop(handle);
        qv_tcp_close(handle);
        free(handle);
        qv_loop_stop(loop);
        return;
    }

    /* Echo the content of HTTP response */
    fwrite(buf, nread, 1, stdout);
    fflush(stdout);

    /* Stop receiving */
    qv_tcp_recv_stop(handle);
    qv_tcp_close(handle);
    free(handle);
    qv_loop_stop(loop);
}

void on_sent(qv_handle_t *handle, int status)
{
    qv_loop_t *loop = handle->loop;

    if (status < 0)
    {
        fprintf(stderr, "error: send() failed\n");
        qv_tcp_close(handle);
        free(handle);
        qv_loop_stop(loop);
        return;
    }
    qv_tcp_recv_start(handle, on_recv);
}

void on_connected(qv_loop_t *loop, qv_handle_t *handle, int status)
{
    char s[BUFFER_SIZE];
    size_t len;

    (void)loop;

    if (status < 0)
    {
        fprintf(stderr, "error: connect() failed\n");
        qv_tcp_close(handle);
        free(handle);
        qv_loop_stop(loop);
        return;
    }
   
    len = sprintf(s, "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "\r\n", g_hostname);

    /* Send request */
    if (qv_tcp_send(handle, s, len, on_sent) < 0)
    {
        qv_tcp_close(handle);
        free(handle);
        qv_loop_stop(loop);
        return;
    }
}

void on_resolved(qv_handle_t *handle, int status, struct addrinfo* res)
{
    qv_loop_t *loop = handle->loop;
    qv_socketaddr_t addr;
    qv_handle_t *client;

    if (status != 0)
    {
        fprintf(stderr, "error: getaddrinfo() failed\n");
        qv_loop_stop(loop);
        return;
    }

    if (qv_socketaddr_init_ipv4_addrinfo(&addr, res) != 0)
    {
        fprintf(stderr, "error: invalid address\n");
        freeaddrinfo(res);
        qv_loop_stop(loop);
        return;
    }
    addr.port = 80;

    printf("%d.%d.%d.%d\n", \
            addr.u.part_ipv4.as_u8[0], \
            addr.u.part_ipv4.as_u8[1], \
            addr.u.part_ipv4.as_u8[2], \
            addr.u.part_ipv4.as_u8[3]);
    fflush(stdout);

    client = (qv_handle_t *)malloc(sizeof(qv_handle_t));
    qv_tcp_init(handle->loop, client);
    if (qv_tcp_connect(client, &addr, on_connected) != 0)
    {
        fprintf(stderr, "error: connect() failed\n");
        freeaddrinfo(res);
        qv_loop_stop(loop);
        return;
    }

    freeaddrinfo(res);
}


int main(int argc, char *argv[])
{
    int ret = 0;
    qv_loop_t loop;
    qv_handle_t resolver;

    /* Initialize memory management */
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

#if defined(QV_PLATFORM_NT)
    qv_winsock_init();
#endif

    /* Initialize event loop */
    if (qv_loop_init(&loop) != 0)
    {
        fprintf(stderr, "error: initialize loop failed\n");
        goto fail;
    }

    if (argc != 2)
    {
        printf("usage: %s <hostname>\n", argv[0]);
        exit(-1);
    }

    g_hostname = argv[1];

    printf("%s is...", argv[1]);
    fflush(stdout);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_canonname = NULL;
    
    if (qv_getaddrinfo(&loop, &resolver, on_resolved, argv[1], "6667", &hints) != 0)
    {
        fprintf(stderr, "error: qv_getaddrinfo()\n");
        exit(-1);
    }

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

