/* qv : Allocator
 * Copyright(c) 2016-2018 y2c2 */

#ifndef QV_ALLOCATOR_H
#define QV_ALLOCATOR_H

#include "qv_types.h"

typedef void *(*qv_malloc_cb_t)(qv_size_t size);
typedef void (*qv_free_cb_t)(void *ptr);

void qv_allocator_set_malloc(qv_malloc_cb_t cb);
void qv_allocator_set_free(qv_free_cb_t cb);

void *qv_malloc(qv_size_t size);
void qv_free(void *ptr);

#endif

