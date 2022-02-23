/* qv : Allocator
 * Copyright(c) 2016-2018 y2c2 */

#include "qv_types.h"
#include "qv_allocator.h"

/* Global stuff */
static qv_malloc_cb_t g_qv_malloc = NULL;
static qv_free_cb_t g_qv_free = NULL;

void qv_allocator_set_malloc(qv_malloc_cb_t cb);
void qv_allocator_set_free(qv_free_cb_t cb);

/* Memory Management */

void qv_allocator_set_malloc(qv_malloc_cb_t cb)
{
    g_qv_malloc = cb;
}

void qv_allocator_set_free(qv_free_cb_t cb)
{
    g_qv_free = cb;
}

void *qv_malloc(qv_size_t size)
{
    return g_qv_malloc(size);
}

void qv_free(void *ptr)
{
    g_qv_free(ptr);
}

