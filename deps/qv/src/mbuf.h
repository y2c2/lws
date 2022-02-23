/* Mutable Buffer
 * Copyright(c) 2016-2018 y2c2 */

#ifndef MBUF_H
#define MBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qv_types.h"

typedef qv_size_t mbuf_size_t;
typedef void *(*mbuf_malloc_cb_t)(mbuf_size_t size);
typedef void (*mbuf_free_cb_t)(void *ptr);
typedef void *(*mbuf_memcpy_cb_t)(void *dest, const void *src, mbuf_size_t n);

struct mbuf
{
    char *body;
    mbuf_size_t size;
    mbuf_size_t capacity;

    mbuf_size_t init_size;
    mbuf_size_t inc_size;
    mbuf_malloc_cb_t malloc_cb;
    mbuf_free_cb_t free_cb;
    mbuf_memcpy_cb_t memcpy_cb;
};

typedef struct mbuf mbuf_t;

int mbuf_init_conf( \
        mbuf_t *mbuf, \
        mbuf_size_t init_size, mbuf_size_t inc_size, \
        mbuf_malloc_cb_t malloc_cb, \
        mbuf_free_cb_t free_cb, \
        mbuf_memcpy_cb_t memcpy_cb);
int mbuf_init( \
        mbuf_t *mbuf, \
        mbuf_malloc_cb_t malloc_cb, \
        mbuf_free_cb_t free_cb, \
        mbuf_memcpy_cb_t memcpy_cb);
void mbuf_uninit(mbuf_t *mbuf);

int mbuf_append(mbuf_t *mbuf, const char *s, const mbuf_size_t len);
int mbuf_append_c_str(mbuf_t *mbuf, const char *s);
int mbuf_shift(mbuf_t *mbuf, const mbuf_size_t len);
int mbuf_clear(mbuf_t *mbuf);

char *mbuf_body(mbuf_t *mbuf);
mbuf_size_t mbuf_size(mbuf_t *mbuf);

#ifdef __cplusplus
}
#endif

#endif


