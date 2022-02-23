/* qv : Handle
 * Copyright(c) 2016 y2c2 */

#ifndef QV_HANDLE_H
#define QV_HANDLE_H

#include "qv_config.h"
#include "qv_types.h"
#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#elif defined(QV_PLATFORM_NT)
#include <Winsock2.h>
#endif

#include "qv_socketaddr.h"
#include "qv_backend.h"
#include "mbuf.h"
#include "pqueue.h"

/* TCP */

typedef enum
{
    /* Unknown */
    QV_TCP_TYPE_UNKNOWN = 0,

    /* Listening
     * (qv_tcp_new_connection_cb_t) */
    QV_TCP_TYPE_LISTENING,

    /* Established 
     * (qv_tcp_recv_cb_t) */
    QV_TCP_TYPE_ESTABLISHED,

    /* Connecting
     * (qv_tcp_connected_cb_t) */
    QV_TCP_TYPE_CONNECTING,
} qv_tcp_type_t;

typedef void (*qv_tcp_new_connection_cb_t)( \
        qv_loop_t *loop, qv_handle_t *server, int status);
typedef void (*qv_tcp_recv_cb_t)( \
        qv_handle_t *handle, qv_ssize_t nread, char *buf);
typedef void (*qv_tcp_send_cb_t)( \
        qv_handle_t *handle, int status);
typedef void (*qv_tcp_error_cb_t)( \
        qv_handle_t *handle, int status);
typedef void (*qv_tcp_connected_cb_t)( \
        qv_loop_t *loop, qv_handle_t *handle, int status);

struct qv_tcp
{
    int fd;
    qv_tcp_type_t type;
    qv_socketaddr_t addr;

    union
    {
        struct
        {
            qv_tcp_new_connection_cb_t new_connection_cb;
        } part_listening;
        struct
        {
            qv_tcp_recv_cb_t recv_cb;
            qv_tcp_send_cb_t send_cb;
            qv_tcp_error_cb_t error_cb;
        } part_established;
        struct
        {
            qv_tcp_connected_cb_t connected_cb;
        } part_connecting;
    } callbacks;

    union
    {
        struct
        {
            mbuf_t send_buffer;
            mbuf_t recv_buffer;
        } part_established;
    } data;
};
typedef struct qv_tcp qv_tcp_t;


/* UDP */

typedef enum
{
    QV_MEMBERSHIP_JOIN,
    QV_MEMBERSHIP_LEAVE,
} qv_membership_t;

typedef enum
{
    /* Unknown */
    QV_UDP_TYPE_UNKNOWN = 0,

    /* Established 
     * (qv_udp_recv_cb_t) */
    QV_UDP_TYPE_ESTABLISHED,

} qv_udp_type_t;

typedef void (*qv_udp_recv_cb_t)( \
        qv_handle_t *handle, qv_ssize_t nread, char *buf, qv_socketaddr_t *addr);

struct qv_udp
{
    int fd;
    qv_udp_type_t type;
    qv_socketaddr_t addr;

    union
    {
        struct
        {
            qv_udp_recv_cb_t recv_cb;
        } part_established;
    } callbacks;

    union
    {
        struct
        {
            pqueue_t *packet_buffer;
        } part_established;
    } data;
};
typedef struct qv_udp qv_udp_t;


/* GetAddrInfo */

typedef void (*qv_getaddrinfo_cb)( \
        qv_handle_t *handle, \
        int status, \
        struct addrinfo* res);

struct qv_getaddrinfo
{
    int ret;
    struct addrinfo* res;
    qv_getaddrinfo_cb getaddrinfo;
};
typedef struct qv_getaddrinfo qv_getaddrinfo_t;

/* GetNameInfo */

typedef void (*qv_getnameinfo_cb)( \
        qv_handle_t *handle, \
        int status, \
        const char *hostname, \
        const char *service);

struct qv_getnameinfo
{
    int ret;
    char *hostname;
    char *service;
    qv_getnameinfo_cb getnameinfo;
};
typedef struct qv_getnameinfo qv_getnameinfo_t;


/* Timer */

typedef qv_u64 qv_interval_t;

typedef void (*qv_timer_cb_t)( \
        qv_handle_t *handle);

typedef enum
{
    QV_TIMER_STATE_AFTER = 0,
    QV_TIMER_STATE_REPEAT = 1
} qv_timer_state_t;

struct qv_timer
{
    qv_timer_state_t state;

    qv_interval_t after;
    qv_interval_t repeat;
    qv_timer_cb_t timer;

    qv_bool again_flag;
};

typedef struct qv_timer qv_timer_t;


/* Process */

typedef void (*qv_process_exit_cb_t)( \
        qv_handle_t *handle, int return_code, int term_signal);

struct qv_process_options
{
    qv_bool defer;
    qv_u32 flags;
    char *file;
    int argc;
    char **argv;
    char **envp;
    char *wd;
    int redir_stdin;
    int redir_stdout;
    int redir_stderr;
    qv_uid_t uid;
    qv_gid_t gid;
    qv_process_exit_cb_t exit_cb;
};
typedef struct qv_process_options qv_process_options_t;

enum
{
    QV_PROCESS_OPTIONS_SET_UID = (1 << 0),
    QV_PROCESS_OPTIONS_SET_GID = (1 << 1),
};

typedef enum
{
    /* Pending to be spawned in thread */
    QV_PROCESS_STATE_SPAWNING = 0,

    /* Running */
    QV_PROCESS_STATE_RUNNING,

    /* Terminated */
    QV_PROCESS_STATE_TERMINATED,

} qv_process_state_t;

struct qv_process
{
    qv_process_state_t state;
    int return_code;
    int term_signal;
    qv_pid_t pid;
    qv_process_options_t options;
	void *process_stub;
};

typedef struct qv_process qv_process_t;


/* Task */

typedef void (*qv_usertask_cb_t)( \
        qv_handle_t *handle, void *data);

struct qv_usertask
{
    qv_usertask_cb_t cb;
    void *data;
};

typedef struct qv_usertask qv_usertask_t;

/* Destructor of attached data */
typedef void (*qv_handle_data_dtor_t)(void *data);

/* Handle */

typedef enum
{
    QV_HANDLE_TYPE_UNKNOWN = 0,
    QV_HANDLE_TYPE_TCP,
    QV_HANDLE_TYPE_UDP,
    QV_HANDLE_TYPE_GETADDRINFO,
    QV_HANDLE_TYPE_GETNAMEINFO,
    QV_HANDLE_TYPE_TIMER,
    QV_HANDLE_TYPE_PROCESS,
    QV_HANDLE_TYPE_USERTASK,
} qv_handle_type_t;

struct qv_handle
{
    qv_loop_t *loop;
    qv_handle_type_t type;
    qv_u32 mode;
    union
    {
        qv_tcp_t tcp;
        qv_udp_t udp;
        qv_getaddrinfo_t getaddrinfo;
        qv_getnameinfo_t getnameinfo;
        qv_timer_t timer;
        qv_process_t process;
        qv_usertask_t usertask;
    } u;

    /* Attached data */
    void *data;
    /* Destructor */
    qv_handle_data_dtor_t data_dtor;
};

#endif

