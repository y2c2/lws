/* qv : Watcher
 * Copyright(c) 2016 y2c2 */

#include "qv_types.h"
#include "qv_config.h"
#include "qv_allocator.h"
#include "qv_handle.h"
#include "qv_watcher.h"

#ifndef MIN
#define MIN(x,y) ((x)>(y)?(x):(y))
#endif

static int qv_backend_fd_cmp(rbt_node_data_t *key1, rbt_node_data_t *key2)
{
    if (key1->fd == key2->fd) return 0;
    return (key1->fd < key2->fd) ? -1 : 1;
}

int qv_watcher_init(qv_watcher_t *watcher)
{
    qv_size_t idx;

    watcher->size = QV_WATCHER_INIT_SIZE;
    watcher->body = (qv_handle_t **)qv_malloc( \
            sizeof(qv_handle_t *) * (QV_WATCHER_INIT_SIZE));
    if (watcher->body == NULL) return -1;

    for (idx = 0; idx != QV_WATCHER_INIT_SIZE; idx++)
    { watcher->body[idx] = NULL; }

    if ((watcher->events = rbt_new( \
                    qv_malloc, \
                    qv_free, \
                    qv_backend_fd_cmp)) == NULL)
    {
        qv_free(watcher->body);
        return -1;
    }
    watcher->used = 0;

    return 0;
}

void qv_watcher_uninit(qv_watcher_t *watcher)
{
    if (watcher->events != NULL)
    {
        rbt_destroy(watcher->events);
        watcher->events = NULL;
    }

    if (watcher->body != NULL)
    {
        qv_free(watcher->body);
        watcher->body = NULL;
    }
}

static qv_size_t roundup_to_power_of_two(qv_size_t x)
{
    x -= 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

static int qv_watcher_extend(qv_watcher_t *watcher, int fd)
{
    qv_handle_t **new_body;
    qv_size_t i;
    qv_size_t new_size;

    /* Unnecessary */
    if (fd < (int)(watcher->size)) { return 0; }

    /* Round up */
    new_size = roundup_to_power_of_two((qv_size_t)fd + 1);
    /* Soft limit */
    new_size = MIN(new_size, QV_WATCHER_VECTOR_SOFT_LIMIT);

    new_body = (qv_handle_t **)qv_malloc( \
            sizeof(qv_handle_t *) * new_size);
    if (new_body == NULL) return -1;

    for (i = 0; i < watcher->size; i++)
    { new_body[i] = watcher->body[i]; }
    for (i = watcher->size; i <  new_size; i++)
    { new_body[i] = NULL; }

    qv_free(watcher->body);
    watcher->body = new_body;
    watcher->size = new_size;

    return 0;
}

int qv_watcher_set(qv_watcher_t *watcher, int fd, qv_handle_t *handle)
{
    rbt_node_data_t key;
    rbt_node_data_t value;

    if (fd >= QV_WATCHER_VECTOR_SOFT_LIMIT)
    {
        /* Exceed soft limit */
        key.fd = fd;
        value.ptr = (void *)handle;
        if (rbt_insert(watcher->events, &key, &value) != 0)
        { return -1; }
    }
    else
    {
        /* Try to extend when necessary */
        if (qv_watcher_extend(watcher, fd) != 0)
        { return -1; }

        /* Insert into vector */ 
        if (watcher->body[fd] != NULL)
        { return -1; }

        watcher->body[fd] = handle;
    }
    watcher->used++;

    return 0;
}

int qv_watcher_clr(qv_watcher_t *watcher, int fd)
{
    rbt_node_data_t key;

    if (fd < (int)watcher->size)
    {
        /* In vector */
        if (watcher->body[fd] == NULL)
        { return -1; }
        watcher->body[fd] = NULL;
    }
    else
    {
        /* In tree */
        key.fd = fd;
        if (rbt_remove(watcher->events, &key) != 0)
        { return -1; }
    }
    watcher->used--;

    return 0;
}

static void *qv_watcher_find_from_tree(rbt_t *events, int fd)
{
    rbt_node_data_t key;
    rbt_node_data_t *result;

    key.fd = fd;
    if (rbt_search( \
                events, \
                &result, \
                &key) != 0)
    { return NULL; }

    return result->ptr;
}

qv_handle_t *qv_watcher_get(qv_watcher_t *watcher, int fd)
{
    if (fd < (int)watcher->size) { return watcher->body[fd]; }

    return qv_watcher_find_from_tree(watcher->events, fd);
}

