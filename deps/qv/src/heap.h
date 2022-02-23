/* Heap */

#ifndef HEAP_H
#define HEAP_H

#include "qv_types.h"
typedef qv_size_t heap_size_t;

struct heap_node_data
{
    int fd;
    void *ptr;
    qv_u32 u32;
    qv_s32 s32;
    qv_u64 u64;
    qv_s64 s64;
};
typedef struct heap_node_data heap_node_data_t;

struct heap_node
{
    heap_node_data_t key;
    heap_node_data_t value;

    struct heap_node *left, *right, *parent;
};
typedef struct heap_node heap_node_t;

/* Callbacks */
typedef int (*heap_cmp_cb_t)(heap_node_data_t *key1, heap_node_data_t *key2);
typedef void *(*heap_malloc_cb_t)(heap_size_t size);
typedef void (*heap_free_cb_t)(void *ptr);

struct heap
{
    heap_node_t *root;

    qv_size_t size;

    heap_malloc_cb_t cb_malloc;
    heap_free_cb_t cb_free;
    heap_cmp_cb_t cb_cmp;
};
typedef struct heap heap_t;

heap_t *heap_new( \
        heap_malloc_cb_t cb_malloc, \
        heap_free_cb_t cb_free, \
        heap_cmp_cb_t cb_cmp);
void heap_destroy(heap_t *rbt);

heap_node_t *heap_root(heap_t *heap);

int heap_insert(heap_t *heap, \
        heap_node_data_t *key, \
        heap_node_data_t *value);
int heap_remove_root(heap_t *heap);

typedef qv_bool (*heap_search_fn)(heap_node_t *heap_node, void *data);

heap_node_t *heap_search_by(heap_t *heap, \
        heap_search_fn fn, \
        void *data);

int heap_remove(heap_t *heap, \
        heap_node_t *heap_node);

#endif

