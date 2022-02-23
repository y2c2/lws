/* qv : LibC
 * Copyright(c) 2016 y2c2 */

#ifndef QV_LIBC_H
#define QV_LIBC_H

#include "qv_types.h"

char *qv_strdup(const char *s);
void *qv_memcpy(void *dest, const void *src, qv_size_t n);
qv_size_t qv_strlen(const char *s);
void *qv_memset(void *s, int c, qv_size_t n);
int qv_strcmp(const char *s1, const char *s2);
int qv_strncmp(const char *s1, const char *s2, qv_size_t n);
const char *qv_strnstrn(const char *haystack, const qv_size_t haystack_len, \
        const char *needle, const qv_size_t needle_len);

#endif

