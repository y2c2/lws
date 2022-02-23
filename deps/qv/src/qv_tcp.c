/* qv : TCP
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"

#include <errno.h>
#include <string.h>
#include <signal.h>

#ifdef QV_PLATFORM_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined(QV_PLATFORM_NT)
#include <Winsock2.h>
#endif

#include "mbuf.h"

#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_tcp.h"
#include "qv_loop.h"
#include "qv_backend.h"
#include "qv_libc.h"
#include "qv_utils.h"


/* Monitor Attributes Graph */

/*
 * As server:
 *
 * None.onListen
 *   -> Listening:input
 *
 * Listening:input.onError
 *   Callback()
 *   -> Dead
 * Listening:input.onInput
 *   Callback()
 * Listening:input.onAccept(self, client)
 *   
 */

static int qv_handle_mode_add(qv_handle_t *handle, qv_u32 event)
{
    if (handle->mode == QV_BACKEND_NONE)
    {
        handle->mode |= event;
        if (qv_backend_ctl( \
                    &handle->loop->backend, \
                    handle->u.tcp.fd, \
                    QV_BACKEND_OP_ADD, \
                    handle->mode, \
                    handle) != 0) \
        { return -1; }
    }
    else if (handle->mode & event) { }
    else
    {
        handle->mode |= event;
        if (qv_backend_ctl( \
                    &handle->loop->backend, \
                    handle->u.tcp.fd, \
                    QV_BACKEND_OP_MOD, \
                    handle->mode, 
                    handle) != 0)
        { return -1; }
    }

    return 0;
}

static int qv_handle_mode_del(qv_handle_t *handle, qv_u32 event)
{
    if (handle->mode == QV_BACKEND_NONE) { }
    else if (!(handle->mode & event)) { }
    else
    {
        handle->mode &= ~((qv_u32)event);
        if (handle->mode == QV_BACKEND_NONE)
        {
            if (qv_backend_ctl( \
                        &handle->loop->backend, \
                        handle->u.tcp.fd, \
                        QV_BACKEND_OP_DEL, \
                        handle->mode, \
                        handle) != 0)
            { return -1; }
        }
        else
        {
            if (qv_backend_ctl( \
                        &handle->loop->backend, \
                        handle->u.tcp.fd, \
                        QV_BACKEND_OP_MOD, \
                        handle->mode, \
                        handle) != 0)
            { return -1; }
        }
    }

    return 0;
}

static int qv_handle_mode_clear(qv_handle_t *handle)
{
    return qv_handle_mode_del(handle, (qv_u32)(~QV_BACKEND_NONE));
}

static void qv_tcp_established_on_unhandled_error(qv_handle_t *handle, int status)
{
    (void)status;

    if (handle->u.tcp.callbacks.part_established.recv_cb != NULL)
    {
        handle->u.tcp.callbacks.part_established.recv_cb(handle, -1, NULL);
    }
    else
    {
        qv_tcp_recv_stop(handle);
        qv_tcp_close(handle);
        return;
    }
}

static int qv_handle_init_established_tcp(qv_handle_t *handle, int fd)
{
    int ret = 0;

    handle->type = QV_HANDLE_TYPE_TCP;
    handle->u.tcp.type = QV_TCP_TYPE_ESTABLISHED;
    handle->u.tcp.fd = fd;
    handle->u.tcp.callbacks.part_established.recv_cb = NULL;
    handle->u.tcp.callbacks.part_established.send_cb = NULL;
    handle->u.tcp.callbacks.part_established.error_cb = qv_tcp_established_on_unhandled_error;

    memset(&handle->u.tcp.data.part_established.send_buffer, 0, sizeof(mbuf_t));
    memset(&handle->u.tcp.data.part_established.recv_buffer, 0, sizeof(mbuf_t));
    if (mbuf_init(&handle->u.tcp.data.part_established.send_buffer, \
                qv_malloc, qv_free, memcpy) != 0)
    { ret = -1; goto fail; }
    if (mbuf_init(&handle->u.tcp.data.part_established.recv_buffer, \
                qv_malloc, qv_free, memcpy) != 0)
    { ret = -1; goto fail; }

fail:
    return ret;
}

int qv_tcp_init(qv_loop_t *loop, qv_handle_t *handle)
{
    handle->type = QV_HANDLE_TYPE_TCP;
    handle->mode = QV_BACKEND_NONE;
    handle->loop = loop;
    qv_memset(&handle->u.tcp, 0, sizeof(qv_tcp_t));
    handle->u.tcp.type = QV_TCP_TYPE_UNKNOWN;
    handle->u.tcp.fd = -1;
    handle->data = NULL;
    handle->data_dtor = NULL;

    return 0;
}

int qv_tcp_bind( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr)
{
    struct sockaddr_in serveraddr;
    qv_u8 *dst_p;

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
    if ((handle->u.tcp.fd = qv_backend_socket_tcp()) < 0)
    { return -1; }
    qv_reuse(handle->u.tcp.fd, qv_true);
    qv_blocking(handle->u.tcp.fd, qv_false);

    /* Bind address */
    if (bind(handle->u.tcp.fd, \
            (struct sockaddr *)&serveraddr, \
            sizeof(serveraddr)) != 0)
    { return -1; }
    memcpy(&handle->u.tcp.addr, addr, sizeof(qv_socketaddr_t));

    return 0;
}

int qv_tcp_listen( \
        qv_handle_t *handle, \
        int backlog, \
        qv_tcp_new_connection_cb_t new_connection_cb)
{
    if (listen(handle->u.tcp.fd, backlog) < 0) return -1;

    handle->u.tcp.type = QV_TCP_TYPE_LISTENING;
    handle->u.tcp.callbacks.part_listening.new_connection_cb = new_connection_cb;

    if (qv_handle_mode_add(handle, QV_BACKEND_INPUT) != 0) return -1;

    return 0;
}

static int qv_tcp_accept_and_close( \
        qv_handle_t *server)
{
    int fd;
    struct sockaddr addr;
    socklen_t addrlen = 0;

    /* Close the stashed fd */
    qv_closesocket(server->loop->stashed_fd);
    server->loop->stashed_fd = -1;

    /* Accept the connection */
    if ((fd = accept(server->u.tcp.fd, &addr, &addrlen)) < 0)
    { goto fail; }

    /* Close the new comming connection */
	qv_closesocket(fd);

fail:
    /* Recreate the stashed fd */
    if ((server->loop->stashed_fd = qv_socket_tcp()) < 0)
    { return -1; }

    return 0;
}

int qv_tcp_accept( \
        qv_handle_t *server, \
        qv_handle_t *client)
{
    int ret = 0;
    int fd = -1;
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);

    /* Clear address */
    memset(&addr, 0, sizeof(struct sockaddr));

    /* Try accepting */
    if ((fd = accept(server->u.tcp.fd, &addr, &addrlen)) < 0)
    {
        if ((fd == -EMFILE) || (fd == -ENFILE))
        {
            /* Too many opened files, accept it and close it immediately */
            if (qv_tcp_accept_and_close(server) != 0)
            { return -1; }
        }
        return -1;
    }
    /* Set as non-blocking */
    qv_blocking(fd, qv_false);

    /* Initialize as established TCP handle */
    if (qv_handle_init_established_tcp(client, fd) != 0)
    { ret = -1; goto fail; }

    /* Address */
    qv_socketaddr_init_ipv4_digits( \
            &client->u.tcp.addr, \
            (char *)(&((struct sockaddr_in *)(&addr))->sin_addr.s_addr), \
            ((struct sockaddr_in *)(&addr))->sin_port);

    /* Monitoring RDHUP to detect if remote side closed */
    if (qv_handle_mode_add(client, QV_BACKEND_RDHUP) != 0)
    { ret = -1; goto fail; }

    goto done;
fail:
    mbuf_uninit(&client->u.tcp.data.part_established.recv_buffer);
    mbuf_uninit(&client->u.tcp.data.part_established.send_buffer);
    if (fd != -1) qv_closesocket(fd);
done:
    return ret;
}

int qv_tcp_connect( \
        qv_handle_t *handle, \
        qv_socketaddr_t *addr, \
        qv_tcp_connected_cb_t cb)
{
    int ret = 0;
    struct sockaddr_in serveraddr;
    qv_u8 *dst_p;

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
    if ((handle->u.tcp.fd = qv_backend_socket_tcp()) < 0)
    { return -1; }
    qv_reuse(handle->u.tcp.fd, qv_true);
    qv_blocking(handle->u.tcp.fd, qv_false);

    /* Connect */
    if ((ret = connect(handle->u.tcp.fd, \
            (struct sockaddr *)&serveraddr, \
            sizeof(serveraddr))) < 0)
    {
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
        int errno1 = errno;
        if (errno1 != EINPROGRESS) { return -1; }
#elif defined(QV_PLATFORM_NT)
        int errno1 = WSAGetLastError();
        if (errno1 != WSAEWOULDBLOCK) { return -1; }
#endif
    }
    memcpy(&handle->u.tcp.addr, addr, sizeof(qv_socketaddr_t));

    handle->u.tcp.type = QV_TCP_TYPE_CONNECTING;
    handle->u.tcp.callbacks.part_connecting.connected_cb = cb;

    /* Monitor writability indicates successfully connected */
    if (qv_handle_mode_add(handle, QV_BACKEND_OUTPUT) != 0) return -1;

    return 0;
}

int qv_tcp_recv_start( \
        qv_handle_t *handle, \
        qv_tcp_recv_cb_t recv_cb)
{
    /* Set callback to mark it as recv */
    if (handle->u.tcp.callbacks.part_established.recv_cb)
    {
        /* recv already started */ 
        return 0;
    }
    handle->u.tcp.callbacks.part_established.recv_cb = recv_cb;

    /* Monitor input */
    if (qv_handle_mode_add(handle, QV_BACKEND_INPUT) != 0) return -1;

    return 0;
}

int qv_tcp_recv_stop(qv_handle_t *handle)
{
    /* Check if already stopped */
    if (handle->u.tcp.callbacks.part_established.recv_cb == NULL)
    { return 0; }

    /* Clear callback */
    handle->u.tcp.callbacks.part_established.recv_cb = NULL;

    /* Stop monitoring input */
    if (qv_handle_mode_del(handle, QV_BACKEND_INPUT) != 0) return -1;

    return 0;
}

int qv_tcp_on_error( \
        qv_handle_t *handle, \
        qv_tcp_error_cb_t error_cb)
{
    handle->u.tcp.callbacks.part_established.error_cb = error_cb;
    return 0;
}

int qv_tcp_send( \
        qv_handle_t *handle, \
        const char *buf, \
        const qv_size_t nsize, \
        qv_tcp_send_cb_t send_cb)
{
    /* Set callback */
    handle->u.tcp.callbacks.part_established.send_cb = send_cb;

    if (mbuf_append( \
                &handle->u.tcp.data.part_established.send_buffer, \
                buf, \
                nsize) != 0)
    { return -1; }

    if (qv_handle_mode_add(handle, QV_BACKEND_OUTPUT) != 0) return -1;

    return 0;
}

int qv_tcp_close( \
        qv_handle_t *handle)
{
    /* Release attached data */
    if (handle->data_dtor != NULL)
    {
        handle->data_dtor(handle->data);
        handle->data = NULL;
        handle->data_dtor = NULL;
    }

    /* Clear buffer */
    mbuf_uninit(&handle->u.tcp.data.part_established.send_buffer);
    mbuf_uninit(&handle->u.tcp.data.part_established.recv_buffer);

    /* Stop watching */
    qv_handle_mode_clear(handle);

    /* Clear activated fd */
    {
        qv_loop_t *loop = handle->loop;
        int i;
        for (i = loop->activated_idx + 1; i < loop->activated_nfds; i++)
        {
            if (loop->activated_fds[i] == handle->u.tcp.fd)
            {
                loop->activated_fds[i] = -1;
                break;
            }
        }
    }

    /* Close socket itself */
    qv_closesocket(handle->u.tcp.fd);

    return 0;
}

static int qv_tcp_process_established_error(qv_handle_t *handle)
{
    qv_tcp_error_cb_t error_cb;

    error_cb = handle->u.tcp.callbacks.part_established.error_cb;

    error_cb(handle, -1);

    return 0;
}

static int qv_tcp_process_established_recv(qv_handle_t *handle, int *closed)
{
    int ret = 0;
    qv_tcp_recv_cb_t recv_cb;
    char buffer[QV_RECV_BUFFER_SIZE];
    qv_ssize_t nread;
    int fd = handle->u.tcp.fd;
    mbuf_t *mbuf_recv;

    mbuf_recv = &handle->u.tcp.data.part_established.recv_buffer;
    recv_cb = handle->u.tcp.callbacks.part_established.recv_cb;

    for (;;)
    {
        nread = recv(fd, buffer, QV_RECV_BUFFER_SIZE, 0);
        if (nread < 0)
        {
            /* Finished reading */
            if (errno == EAGAIN) { break; }

            /* Error */
            *closed = 1;
            if (recv_cb != NULL) recv_cb(handle, -1, NULL);
            goto fail;
        }
        else if (nread == 0)
        {
            /* Finished reading */
            break;
        }
        else
        {
            if (mbuf_append(mbuf_recv, buffer, (qv_size_t)nread) < 0)
            {
                if (recv_cb != NULL) recv_cb(handle, -1, NULL);
                goto fail;
            }

            /* Finished reading */ 
            if (nread < QV_RECV_BUFFER_SIZE) break;
        }
    }

    /* No data received, probably due to the close of client */
    if (nread == 0)
    {
        /* Error */
        *closed = 1;
        if (recv_cb != NULL) recv_cb(handle, -1, mbuf_body(mbuf_recv));
        goto fail;
    }

    /* Success */
    if (recv_cb != NULL)
    {
        /* Has handler */
        {
            /* Throw out the receive buffer */
            char *buf; qv_ssize_t len = (qv_ssize_t)mbuf_size(mbuf_recv);
            if ((buf = (char *)qv_malloc(sizeof(char) * (qv_size_t)len + 1)) == NULL)
            {
                *closed = 1;
                if (recv_cb != NULL) recv_cb(handle, -1, NULL);
                goto fail;
            }
            qv_memcpy(buf, mbuf_body(mbuf_recv), (qv_size_t)len);
            mbuf_clear(mbuf_recv);
            recv_cb(handle, len, buf);
            qv_free(buf);
        }
    }
    else
    {
        /* No handler, temporary save it */
    }

    goto done;
fail:
    ret = -1;
done:
    return ret;
}
static void sighandler_sigpipe(int num)
{
    (void)num;
}

static int qv_tcp_process_established_send(qv_handle_t *handle)
{
    int fd = handle->u.tcp.fd;
    qv_ssize_t sent_size;
    mbuf_t *send_buffer = &handle->u.tcp.data.part_established.send_buffer;

    while (mbuf_size(send_buffer) > 0)
    {
        /* Handle SIGPIPE */
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
        signal(SIGPIPE, sighandler_sigpipe);
#else
        (void)sighandler_sigpipe;
#endif
        sent_size = send(fd, mbuf_body(send_buffer), mbuf_size(send_buffer), 0);
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
		signal(SIGPIPE, NULL);
#endif
		/* Restore SIGPIPE handler */
        if (sent_size < 0) 
        {
            /* This call may return a non 0 integer to indicate an error */
            mbuf_clear(send_buffer);

            /* Invoke error callback (Release resource in callback) */
            handle->u.tcp.callbacks.part_established.send_cb(handle, -1);
            return -1;
        }
        else if (sent_size == 0) break;
        else
        {
            mbuf_shift(send_buffer, (qv_size_t)sent_size);
        }

        if (qv_is_writable(fd) == 0) { break; }
    }

    /* Stop watching, when finished sending all data. */
    if (mbuf_size(send_buffer) == 0)
    {
        qv_handle_mode_del(handle, QV_BACKEND_OUTPUT);
    }

    /* Finished sending callback */
    if (handle->u.tcp.callbacks.part_established.send_cb == NULL)
    { return -1; }
    mbuf_clear(send_buffer);
    handle->u.tcp.callbacks.part_established.send_cb(handle, 0);

    return 0;
}

static int qv_tcp_process_connecting_connected(qv_handle_t *handle)
{
    int err = 0;
    socklen_t len = sizeof(int);
    qv_tcp_connected_cb_t connected_cb;

    connected_cb = handle->u.tcp.callbacks.part_connecting.connected_cb;

    /* Distinguish error or connected */
    if (getsockopt(handle->u.tcp.fd, SOL_SOCKET, SO_ERROR, (void *)&err, &len) == -1)
    { goto fail; }

    /* Initialize as established TCP handle */
    if (qv_handle_init_established_tcp(handle, handle->u.tcp.fd) != 0)
    { goto fail; }

    /* Callback */
    connected_cb(handle->loop, handle, 0);

    return 0;

fail:
    /* Callback */
    handle->u.tcp.callbacks.part_connecting.connected_cb(handle->loop, handle, -1);
    return 0;
}

static int qv_tcp_process_connecting_error(qv_handle_t *handle)
{
    handle->u.tcp.type = QV_TCP_TYPE_UNKNOWN;
    handle->u.tcp.callbacks.part_connecting.connected_cb(handle->loop, handle, -1);

    return 0;
}

int qv_tcp_process(qv_handle_t *handle, qv_u32 events)
{
    int ret = 0;
    int closed;

    switch (handle->u.tcp.type)
    {
        case QV_TCP_TYPE_UNKNOWN:
            break;

        case QV_TCP_TYPE_LISTENING:
            if ((events & QV_BACKEND_ERR) || (events & QV_BACKEND_HUP) || (!(events & QV_BACKEND_INPUT)))
            { qv_closesocket(handle->u.tcp.fd); return 0; }
            else
            { handle->u.tcp.callbacks.part_listening.new_connection_cb(handle->loop, handle, 0); }
            break;

        case QV_TCP_TYPE_ESTABLISHED:
            if ((events & QV_BACKEND_ERR) || (events & QV_BACKEND_HUP))
            {
                /* Error Handling */
                qv_tcp_process_established_error(handle);
            }
            else 
            {
                /* Marked as not closed */
                closed = 0;
                if (events & QV_BACKEND_INPUT)
                { qv_tcp_process_established_recv(handle, &closed); }

                if ((closed == 0) && (events & QV_BACKEND_OUTPUT))
                { qv_tcp_process_established_send(handle); }
            }
            break;

        case QV_TCP_TYPE_CONNECTING:
            if (qv_handle_mode_del(handle, QV_BACKEND_OUTPUT) != 0) return -1;
            if ((events & QV_BACKEND_ERR) || (events & QV_BACKEND_HUP))
            {
                qv_tcp_process_connecting_error(handle);
            }
            else if (events & QV_BACKEND_OUTPUT)
            {
                qv_tcp_process_connecting_connected(handle);
            }
            break;
    }

    return ret;
}

