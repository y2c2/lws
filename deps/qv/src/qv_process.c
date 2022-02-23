/* qv : Process
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"
#include "qv_loop.h"
#include "qv_handle.h"
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
#include "qv_unixprocess.h"
#elif defined(QV_PLATFORM_NT)
#include "qv_ntprocess.h"
#endif
#include "qv_process.h"

int qv_process_spawn( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_process_options_t *options)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_unixprocess_spawn(loop, handle, options);
#elif defined(QV_PLATFORM_NT)
	return qv_ntprocess_spawn(loop, handle, options);
#endif
}

int qv_process_kill( \
        qv_handle_t *handle, int signum)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    return qv_unixprocess_kill(handle, signum);
#elif defined(QV_PLATFORM_NT)
    return qv_ntprocess_kill(handle, signum);
#endif
}

