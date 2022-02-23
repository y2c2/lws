#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <qv.h>

void process_exit_callback( \
        qv_handle_t *handle, int return_code, int term_signal)
{
    qv_loop_t *loop = handle->loop;
    (void)term_signal;

    printf("return code = %d\n", return_code);
    free(handle);
    qv_loop_stop(loop);
}

int main(void)
{
    int ret = 0;
    qv_loop_t loop;
    qv_process_options_t options;
    qv_handle_t *process;
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    static char *options_file = "/bin/uname";
    static char *options_argv[] = {"/bin/uname", NULL};
#elif defined(QV_PLATFORM_NT)
    static char *options_file = "C:\\Windows\\notepad.exe";
    static char *options_argv[] = {"C:\\Windows\\notepad.exe", NULL};
#endif

#if defined(QV_PLATFORM_NT)
    qv_winsock_init();
#endif

    /* Initialize memory management */
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

    /* Initialize event loop */
    if (qv_loop_init(&loop) != 0)
    {
        fprintf(stderr, "error: initialize loop failed\n");
        goto fail;
    }

    /* Initialize Process */
    process = (qv_handle_t *)malloc(sizeof(qv_handle_t));
    memset(&options, 0, sizeof(qv_process_options_t));

    options.defer = qv_true;

    options.exit_cb = process_exit_callback;
    options.file = options_file;
    options.argv = options_argv;
    options.argc = 1;
    qv_process_spawn(&loop, process, &options);

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

