/* Chrono */

#include "qv_config.h"
#include "chrono.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include <time.h>
#elif defined(QV_PLATFORM_MACOS)
#include <time.h>
#elif defined(QV_PLATFORM_NT)
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#include <Windows.h>
#endif
int chrono_now(chrono_time_point_t *time_point)
{
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
    struct timespec tc;
    if (clock_gettime(CLOCK_REALTIME, &tc) != 0)
    { return -1; }
    *time_point = (chrono_time_point_t)(tc.tv_sec * 1000 + tc.tv_nsec / 1000000);
    return 0;
#elif defined(QV_PLATFORM_NT)
    *time_point = (chrono_time_point_t)GetTickCount64();
    return 0;
#endif
}

