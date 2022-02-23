/* qv : Utilities
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"
#include "qv_backend.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(QV_PLATFORM_NT)
#include <WinSock2.h>
#include <Windows.h>
#endif

#include "qv_utils.h"

int qv_reuse(int fd, qv_bool enabled)
{
    int optval;
    optval = enabled ? 1 : 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
            (const void *)&optval , sizeof(int));
    return 0;
}

int qv_blocking(int fd, qv_bool enabled)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
   int flags = fcntl(fd, F_GETFL, 0);
   flags = enabled ? (flags &~ O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? 0 : -1;
#elif defined(QV_PLATFORM_NT)
	ULONG NonBlock = enabled ? 0 : 1;
	if (ioctlsocket(fd, FIONBIO, &NonBlock) == SOCKET_ERROR) return -1;
	return 0;
#endif
}

int qv_is_readable(int fd)
{
    fd_set fd_read;
    struct timeval timeout = {0, 1};
    int retval;

    FD_ZERO(&fd_read);
    FD_SET(fd, &fd_read);

    retval = select(fd + 1, &fd_read, NULL, NULL, &timeout);
    return retval > 0 ? 1 : 0;
}

int qv_is_writable(int fd)
{
    fd_set fd_write;
    struct timeval timeout = {0, 1};
    int retval;

    FD_ZERO(&fd_write);
    FD_SET(fd, &fd_write);

    retval = select(fd + 1, &fd_write, NULL, NULL, &timeout);
    return retval > 0 ? 1 : 0;
}

int qv_socket_tcp(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int qv_closesocket(int fd)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    close(fd);
#elif defined(QV_PLATFORM_NT)
    closesocket(fd);
#endif
    return 0;
}

void qv_usleep(unsigned int usec)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    usleep(usec);
#elif defined(QV_PLATFORM_NT)
    Sleep(usec * 1000);
#endif

}

