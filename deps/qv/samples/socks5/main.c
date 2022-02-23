/* Socks5 Proxy
 * Copyright(c) 2017 y2c2 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <qv.h>
#include "socks5.h"

#define DEFAULT_SERVER_HOST "127.0.0.1"
#define DEFAULT_SERVER_PORT 1080

extern qv_loop_t *g_loop;

void sighandler(int num)
{
    if (num == SIGINT) 
    {
        fprintf(stderr, "Control-C received, exit...\n");
        qv_loop_stop(g_loop);
    }
}

int main(void)
{
    int ret = 0;

    socks5_startup startup;

#if defined(QV_PLATFORM_NT)
    qv_winsock_init();
#endif

    signal(SIGINT, sighandler);
    startup.bound_address = DEFAULT_SERVER_HOST;
    startup.bound_port = DEFAULT_SERVER_PORT;
    if (socks5_start(&startup) != 0)
    {
        goto fail;
    }

    goto done;
fail:
    ret = -1;
done:
#if defined(QV_PLATFORM_NT)
    qv_winsock_uninit();
#endif
    return ret;
}

