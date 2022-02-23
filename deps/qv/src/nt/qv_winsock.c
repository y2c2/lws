/* qv : Winsock
 * Copyright(c) 2016 y2c2 */

#include <Winsock2.h>

#include "qv_winsock.h"

int qv_winsock_init(void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) { return -1; }

    return 0;
}

int qv_winsock_uninit(void)
{
    WSACleanup();

	return 0;
}

