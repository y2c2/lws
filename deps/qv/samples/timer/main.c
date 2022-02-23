#include <stdio.h>
#include <stdlib.h>
#include <qv.h>

static volatile int counter = 0;


void timer_callback(qv_handle_t *handle)
{
    qv_loop_t *loop = handle->loop;

    if (counter++ >= 5) qv_loop_stop(loop);

    printf("Hello World\n");

    qv_timer_again(handle);
}

int main(void)
{
    int ret = 0;
    qv_loop_t loop;
    qv_handle_t timer;

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

    /* Initialize Timer */
    qv_timer_init(&loop, &timer);
    qv_timer_set(&timer, timer_callback, 3000, 1000);
    qv_timer_start(&timer);

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

