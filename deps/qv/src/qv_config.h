/* qv : Configure
 * Copyright(c) 2016 y2c2 */

#ifndef QV_CONFIG_H
#define QV_CONFIG_H

/* Platform */

/*
#define QV_PLATFORM_NT
#define QV_PLATFORM_LINUX
#define QV_PLATFORM_BSD
#define QV_PLATFORM_MACOS
*/

#if defined(linux)
# define QV_PLATFORM_LINUX
#elif defined(__FreeBSD__)
# define QV_PLATFORM_FREEBSD
#elif defined(__APPLE__)
# define QV_PLATFORM_MACOS
#elif (defined(__GNUC__) && \
        (defined(__MINGW32__) || \
         defined(__MINGW64__) || \
         defined(__MSYS__) || \
         defined(__CYGWIN__))) || \
         defined(_MSC_VER)
# define QV_PLATFORM_NT
#else
# define QV_PLATFORM_UNKNOWN
#endif

#ifdef QV_PLATFORM_NT
# define WIN32_LEAN_AND_MEAN
#endif

/* Backend
 *
 * Some events are only available on some platforms
 * select the proper one according to your platform. */

/* #define QV_BACKEND_SELECT */

/*
#define QV_BACKEND_SELECT
#define QV_BACKEND_POLL
#define QV_BACKEND_KQUEUE
#define QV_BACKEND_IOCP
#define QV_BACKEND_EPOLL
*/

#ifndef QV_BACKEND_SELECT
# ifndef QV_BACKEND_POLL
#  ifndef QV_BACKEND_KQUEUE
#   ifndef QV_BACKEND_IOCP
#    ifndef QV_BACKEND_EPOLL
#     define QV_BACKEND_UNDEFINED
#    endif
#   endif
#  endif
# endif
#endif

#ifdef QV_BACKEND_UNDEFINED
# if defined(QV_PLATFORM_LINUX)
#  define QV_BACKEND_EPOLL
# elif defined(QV_PLATFORM_FREEBSD)
#  define QV_BACKEND_SELECT
# elif defined(QV_PLATFORM_MACOS)
#  define QV_BACKEND_SELECT
# elif defined(QV_PLATFORM_NT)
#  define QV_BACKEND_SELECT
# else
#  define QV_BACKEND_SELECT
# endif
# undef QV_BACKEND_UNDEFINED
#endif

/* Maximum events watching at same time */
#define QV_WATCHER_MAXEVENTS 64

/* Watcher vector initial size */
#define QV_WATCHER_INIT_SIZE 128

/* Watcher vector maximum size */
#define QV_WATCHER_VECTOR_SOFT_LIMIT 1024

/* Receiving buffer size */
#define QV_RECV_BUFFER_SIZE 4096

/* Number of worker */
#define QV_WORKER_NUM 1

#endif

