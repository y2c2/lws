/* qv : Types
 * Copyright(c) 2016 y2c2 */

#ifndef QV_TYPES_H
#define QV_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qv_config.h"

/* Primitive Types */

#ifndef __cplusplus 
typedef enum
{
    qv_false = 0,
    qv_true = 1,
} qv_bool;
#else
typedef enum
{
    qv_false = false,
    qv_true = true,
} qv_bool;
#endif

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
#include <stdio.h>
typedef uint64_t qv_u64;
typedef uint32_t qv_u32;
typedef int32_t qv_s32;
typedef int64_t qv_s64;
typedef uint16_t qv_u16;
typedef int16_t qv_s16;
typedef uint8_t qv_u8;
typedef int8_t qv_s8;
typedef size_t qv_size_t;
typedef ssize_t qv_ssize_t;
#else
typedef unsigned long long qv_u64;
typedef unsigned int qv_u32;
typedef unsigned short int qv_u16;
typedef unsigned char qv_u8;
typedef signed long long qv_s64;
typedef signed int qv_s32;
typedef signed short int qv_s16;
typedef char qv_s8;
typedef unsigned int qv_size_t;
typedef signed int qv_ssize_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* qv Types */

struct qv_loop;
typedef struct qv_loop qv_loop_t;
struct qv_socket;
typedef struct qv_socket qv_socket_t;
struct qv_socket_addr_ipv4;
typedef struct qv_socket_addr_ipv4 qv_socket_addr_ipv4_t;
typedef int qv_port_t;
struct qv_handle;
typedef struct qv_handle qv_handle_t;

/* Platform specified types */
#if defined(QV_PLATFORM_NT)
typedef int socklen_t;
#endif

#if defined(QV_PLATFORM_NT)
typedef unsigned int qv_uid_t;
typedef unsigned int qv_gid_t;
typedef unsigned int qv_pid_t;
#elif defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD) || defined(QV_PLATFORM_MACOS)
#include <sys/types.h>
typedef uid_t qv_uid_t;
typedef gid_t qv_gid_t;
typedef pid_t qv_pid_t;
#endif

#ifdef __cplusplus
}
#endif


#endif


