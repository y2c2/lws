#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qv.h>

void on_resolved(qv_handle_t *handle, int status, struct addrinfo* res)
{
    qv_socketaddr_t addr;

    if (status != 0)
    {
        fprintf(stderr, "error: getaddrinfo() failed\n");
        qv_loop_stop(handle->loop);
        return;
    }

    if (qv_socketaddr_init_ipv4_addrinfo(&addr, res) != 0)
    {
        fprintf(stderr, "error: invalid address\n");
        freeaddrinfo(res);
        qv_loop_stop(handle->loop);
        return;
    }

    printf("%d.%d.%d.%d\n", \
            addr.u.part_ipv4.as_u8[0], \
            addr.u.part_ipv4.as_u8[1], \
            addr.u.part_ipv4.as_u8[2], \
            addr.u.part_ipv4.as_u8[3]);

    freeaddrinfo(res);

    qv_loop_stop(handle->loop);
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

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_canonname = NULL;

    printf("%s is...", argv[1]);
    fflush(stdout);
    
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
