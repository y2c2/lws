/* qv : Utilities
 * Copyright(c) 2016 y2c2 */

#ifndef QV_UTILS_H
#define QV_UTILS_H

#include "qv_types.h"

int qv_reuse(int fd, qv_bool enabled);

int qv_blocking(int fd, qv_bool enabled);

int qv_is_readable(int fd);

int qv_is_writable(int fd);

int qv_socket_tcp(void);

int qv_closesocket(int fd);

void qv_usleep(unsigned int usec);

#endif

