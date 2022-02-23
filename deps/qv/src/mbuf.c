/* Mutable Buffer
 * Copyright(c) 2016-2018 y2c2 */

#include "qv_libc.h"
#include "mbuf.h"

#define MBUF_DEFAULT_INIT_SIZE 512
#define MBUF_DEFAULT_INC_SIZE 512

int mbuf_init_conf( \
        mbuf_t *mbuf, \
        mbuf_size_t init_size, mbuf_size_t inc_size, \
        mbuf_malloc_cb_t malloc_cb, \
        mbuf_free_cb_t free_cb, \
        mbuf_memcpy_cb_t memcpy_cb)
{
    mbuf->malloc_cb = malloc_cb;
    mbuf->free_cb = free_cb;
    mbuf->memcpy_cb = memcpy_cb;
    mbuf->size = 0;
    mbuf->capacity = init_size;
    mbuf->init_size = init_size;
    mbuf->inc_size = inc_size;
    if ((mbuf->body = (char *)malloc_cb( \
                    sizeof(char) * init_size)) == ((void *)0))
    { return -1; }

    return 0;
}

int mbuf_init( \
        mbuf_t *mbuf, \
        mbuf_malloc_cb_t malloc_cb, \
        mbuf_free_cb_t free_cb, \
        mbuf_memcpy_cb_t memcpy_cb)
{
    return mbuf_init_conf( \
            mbuf, \
            MBUF_DEFAULT_INIT_SIZE, \
            MBUF_DEFAULT_INC_SIZE, \
            malloc_cb, \
            free_cb, \
            memcpy_cb);
}

void mbuf_uninit(mbuf_t *mbuf)
{
    if (mbuf->body != (void *)0)
    {
        mbuf->free_cb(mbuf->body);
        mbuf->body = (void *)0;
    }
}

int mbuf_append(mbuf_t *mbuf, const char *s, const mbuf_size_t len)
{
    char *new_buf = (void *)0;
    mbuf_size_t new_capacity;

    if (mbuf->size + len + 1 >= mbuf->capacity)
    {
        /* Extend */
        new_capacity = mbuf->size + len + 1 + mbuf->inc_size;
        new_buf = (char *)mbuf->malloc_cb( \
                sizeof(char) * new_capacity);
        if (new_buf == (void *)0) return -1;
        if (mbuf->size > 0)
        {
            mbuf->memcpy_cb(new_buf, mbuf->body, mbuf->size);
        }
        mbuf->memcpy_cb(new_buf + mbuf->size, s, len);
        mbuf->size = mbuf->size + len;
        new_buf[mbuf->size] = '\0';
        mbuf->capacity = new_capacity;
        mbuf->free_cb(mbuf->body);
        mbuf->body = new_buf;
    }
    else
    {
        mbuf->memcpy_cb(mbuf->body + mbuf->size, s, len);
        mbuf->size += len;
        mbuf->body[mbuf->size] = '\0';
    }

    return 0;
}

int mbuf_append_c_str(mbuf_t *mbuf, const char *s)
{
    return mbuf_append(mbuf, s, qv_strlen(s));
}

int mbuf_shift(mbuf_t *mbuf, const mbuf_size_t len)
{
    if (len > mbuf->size) return -1;

    mbuf->memcpy_cb(mbuf->body, mbuf->body + len, mbuf->size - len);
    mbuf->size -= len;

    return 0;
}

int mbuf_clear(mbuf_t *mbuf)
{
    mbuf->free_cb(mbuf->body);
    if ((mbuf->body = (char *)mbuf->malloc_cb( \
                    sizeof(char) * mbuf->init_size)) == ((void *)0))
    { return -1; }
    mbuf->size = 0;
    mbuf->capacity = mbuf->init_size;

    return 0;
}

char *mbuf_body(mbuf_t *mbuf)
{
    return mbuf->body;
}

mbuf_size_t mbuf_size(mbuf_t *mbuf)
{
    return mbuf->size;
}

