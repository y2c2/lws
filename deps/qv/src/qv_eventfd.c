/* qv : Event File Descriptor
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"
#if defined(QV_PLATFORM_LINUX)
#include <unistd.h>
#elif defined(QV_PLATFORM_NT)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#define QV_EVENTFD_PAIR_PORT 27903
#elif defined(QV_PLATFORM_MACOS) || defined(QV_PLATFORM_FREEBSD)
#define QV_EVENTFD_PAIR_PORT 27903
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#endif
#include "qv_libc.h"
#include "qv_utils.h"
#include "qv_eventfd.h"

/* The 'qv_eventfd_new' produces 2 sockets
 * Sender - To signal the waking action
 * Receiver - To be monitored by pool (RETURN VALUE)
 */
qv_eventfd qv_eventfd_new(qv_eventfd *sender)
{
#if defined(QV_PLATFORM_LINUX)
    qv_eventfd efd = eventfd(0, EFD_CLOEXEC);
    if (efd == -1) return QV_EVENTFD_INVALID;
    *sender = efd;
    return efd;
#elif defined(QV_PLATFORM_NT) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    struct sockaddr_in serveraddr;
    qv_eventfd master_fd = -1;
    qv_eventfd received_fd = -1, sender_fd = -1;
    master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (master_fd == -1) return QV_EVENTFD_INVALID;
    {
        char optval = 1;
        setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    }
    {
        qv_memset((char *)&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        serveraddr.sin_port = htons((unsigned short)QV_EVENTFD_PAIR_PORT);
        if (bind(master_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        { qv_closesocket(master_fd); return QV_EVENTFD_INVALID; }
        if (listen(master_fd, 5) < 0)
        { qv_closesocket(master_fd); return QV_EVENTFD_INVALID; }
    }
    {
        if ((sender_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        { qv_closesocket(master_fd); return QV_EVENTFD_INVALID; }
        if (qv_blocking(sender_fd, qv_false) != 0)
        { qv_closesocket(master_fd); qv_closesocket(sender_fd); return QV_EVENTFD_INVALID; }
        if (connect(sender_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        {
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
            int errno1 = errno;
            if (errno1 != EINPROGRESS)
            { qv_closesocket(master_fd); qv_closesocket(sender_fd); return QV_EVENTFD_INVALID; }
#elif defined(QV_PLATFORM_NT)
            int errno1 = WSAGetLastError();
            if (errno1 != WSAEWOULDBLOCK)
            { qv_closesocket(master_fd); qv_closesocket(sender_fd); return QV_EVENTFD_INVALID; }
#endif
        }
    }
    {
        struct sockaddr_in clientaddr;
        socklen_t clientlen = sizeof(clientaddr);
        received_fd = accept(master_fd, (struct sockaddr *)&clientaddr, &clientlen);
        if (received_fd < 0)
        { qv_closesocket(master_fd); qv_closesocket(sender_fd); return QV_EVENTFD_INVALID; }
    }
    *sender = sender_fd;
    return received_fd;
#endif
}

void qv_eventfd_close(qv_eventfd efd)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    close(efd);
#elif defined(QV_PLATFORM_NT)
    closesocket(efd);
#endif
}

int qv_eventfd_write(qv_eventfd efd, qv_u64 value)
{
#if defined(QV_PLATFORM_LINUX)
    if (eventfd_write(efd, value) < 0) return -1;
#elif defined(QV_PLATFORM_NT) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    if (send(efd, (const char *)&value, sizeof(qv_u64), 0) < 0) return -1;
#endif
    return 0;
}

int qv_eventfd_read(qv_eventfd efd, qv_u64 *value)
{
#if defined(QV_PLATFORM_LINUX)
    if (eventfd_read(efd, value) < 0) return -1;
#elif defined(QV_PLATFORM_NT) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    if (recv(efd, (char *)value, sizeof(qv_u64), 0) < 0) return -1;
#endif
    return 0;
}

