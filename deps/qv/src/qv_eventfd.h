/* qv : Event File Descriptor
 * Copyright(c) 2016 y2c2 */

#ifndef QV_EVENTFD_H
#define QV_EVENTFD_H

#include "qv_config.h"
#include "qv_types.h"

#if defined(QV_PLATFORM_LINUX)

#include <sys/eventfd.h>
typedef int qv_eventfd;
#define QV_EVENTFD_INVALID (-1)

#elif defined(QV_PLATFORM_NT)

#include <windows.h>
typedef int qv_eventfd;
#define QV_EVENTFD_INVALID (-1)

#elif defined(QV_PLATFORM_MACOS) || defined(QV_PLATFORM_FREEBSD)

typedef int qv_eventfd;
#define QV_EVENTFD_INVALID (-1)

#endif

qv_eventfd qv_eventfd_new(qv_eventfd *sender);
void qv_eventfd_close(qv_eventfd efd);
int qv_eventfd_write(qv_eventfd efd, qv_u64 value);
int qv_eventfd_read(qv_eventfd efd, qv_u64 *value);

#endif

