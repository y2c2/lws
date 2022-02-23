/* Red-Black Tree */

/* The implementation of Red-Black Tree in
 * <<Introduction of Algorithms Second Edition>> by CLRS */

#ifndef RBT_H
#define RBT_H

#include "qv_types.h"
typedef qv_size_t rbt_size_t;

/* Node */

typedef enum 
{
    RBT_NODE_COLOR_RED,
    RBT_NODE_COLOR_BLACK,
} rbt_node_color_t;

typedef union
{
    int fd;
    void *ptr;
    qv_u32 u32;
    qv_s32 s32;
} rbt_node_data_t;

struct rbt_node
{
    rbt_node_data_t key;
    rbt_node_data_t value;

    struct rbt_node *left, *right, *parent;
    rbt_node_color_t color;
};
typedef struct rbt_node rbt_node_t;

/* Callbacks */
typedef int (*rbt_cmp_cb_t)(rbt_node_data_t *key1, rbt_node_data_t *key2);
typedef void *(*rbt_malloc_cb_t)(rbt_size_t size);
typedef void (*rbt_free_cb_t)(void *ptr);

/* Tree */

struct rbt
{
    rbt_node_t *root;
    rbt_node_t *nil;

    rbt_cmp_cb_t cb_cmp;
    rbt_malloc_cb_t cb_malloc;
    rbt_free_cb_t cb_free;
};
typedef struct rbt rbt_t;

rbt_t *rbt_new( \
        rbt_malloc_cb_t cb_malloc, \
        rbt_free_cb_t cb_free, \
        rbt_cmp_cb_t cb_cmp);
void rbt_destroy(rbt_t *rbt);


/* Operations */

int rbt_insert(rbt_t *rbt, void *key, void *value);
int rbt_search( \
        rbt_t *rbt, \
        rbt_node_data_t **value_out, \
        rbt_node_data_t *key);
rbt_node_t *rbt_minimum(rbt_t *rbt, rbt_node_t *x);
rbt_node_t *rbt_maximum(rbt_t *rbt, rbt_node_t *x);
rbt_node_t *rbt_successor(rbt_t *rbt, rbt_node_t *x);
rbt_node_t *rbt_predecessor(rbt_t *rbt, rbt_node_t *x);
rbt_node_t *rbt_remove(rbt_t *rbt, void *key);


/* Iterator */

typedef struct rbt_iterator
{
    rbt_node_t *node_cur;
    rbt_t *rbt;
} rbt_iterator_t;

int rbt_iterator_init(rbt_t *rbt, rbt_iterator_t *iterator);
void rbt_iterator_next(rbt_iterator_t *iterator);
rbt_node_t *rbt_iterator_deref(rbt_iterator_t *iterator);
rbt_node_data_t *rbt_iterator_deref_key(rbt_iterator_t *iterator);
rbt_node_data_t *rbt_iterator_deref_value(rbt_iterator_t *iterator);
int rbt_iterator_isend(rbt_iterator_t *iterator);

typedef int (*rbt_iterator_callback_t)( \
        rbt_node_data_t *key, \
        rbt_node_data_t *value, \
        void *data);

int rbt_iterate(rbt_t *rbt, \
        rbt_iterator_callback_t callback, \
        void *data);


#endif

