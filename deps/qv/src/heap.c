/* Heap */

#include "qv_libc.h"
#include "heap.h"

static heap_node_t *heap_node_new(heap_t *heap, \
        heap_node_data_t *key, \
        heap_node_data_t *value)
{
    heap_node_t *new_node = (heap_node_t *)heap->cb_malloc( \
            sizeof(heap_node_t));
    if (new_node == NULL) return NULL;
    new_node->parent = new_node->left = new_node->right = NULL;
    qv_memcpy(&new_node->key, key, sizeof(heap_node_data_t));
    qv_memcpy(&new_node->value, value, sizeof(heap_node_data_t));

    return new_node;
}

static void heap_node_destroy(heap_t *heap, \
        heap_node_t *node)
{
    if (node->left != NULL) heap_node_destroy(heap, node->left);
    if (node->right != NULL) heap_node_destroy(heap, node->right);
    heap->cb_free(node);
}

heap_t *heap_new( \
        heap_malloc_cb_t cb_malloc, \
        heap_free_cb_t cb_free, \
        heap_cmp_cb_t cb_cmp)
{
    heap_t *new_heap = (heap_t *)cb_malloc( \
            sizeof(heap_t));
    if (new_heap == NULL) return NULL;

    /* Allocators */
    new_heap->cb_malloc = cb_malloc;
    new_heap->cb_free = cb_free;
    if (cb_cmp == NULL)
    { new_heap->cb_cmp = NULL; }
    else
    { new_heap->cb_cmp = cb_cmp; }

    new_heap->size = 0;
    new_heap->root = NULL;

    return new_heap;
}

void heap_destroy(heap_t *heap)
{
    if (heap->root != NULL) heap_node_destroy(heap, heap->root);
    heap->cb_free(heap);
}

static void heap_move_nodes( \
        heap_node_t *node_dst, \
        heap_node_t *node_src)
{
    qv_memcpy(&node_dst->key, &node_src->key, \
            sizeof(heap_node_data_t));
    qv_memcpy(&node_dst->value, &node_src->value, \
            sizeof(heap_node_data_t));
}

static void heap_swap_nodes( \
        heap_node_t *node1, \
        heap_node_t *node2)
{
    heap_node_t node_tmp;

    qv_memcpy(&node_tmp.key, &node1->key, sizeof(heap_node_data_t));
    qv_memcpy(&node1->key, &node2->key, sizeof(heap_node_data_t));
    qv_memcpy(&node2->key, &node_tmp.key, sizeof(heap_node_data_t));

    qv_memcpy(&node_tmp.value, &node1->value, sizeof(heap_node_data_t));
    qv_memcpy(&node1->value, &node2->value, sizeof(heap_node_data_t));
    qv_memcpy(&node2->value, &node_tmp.value, sizeof(heap_node_data_t));
}

int heap_insert(heap_t *heap, \
        heap_node_data_t *key, \
        heap_node_data_t *value)
{
    heap_node_t *node_cur;
    heap_node_t *new_node;
    qv_u32 path = 0, level = 0;
    qv_size_t n;

    new_node = heap_node_new(heap, key, value);
    if (new_node == NULL) return -1;

    if (heap->root == NULL)
    {
        /* Empty heap */
        heap->root = new_node;
    }
    else
    {
        /* Search the position at last level of heap */
        n = heap->size + 1;
        while (n >= 2)
        {
            path = (path << 1) | (n & 1);

            level += 1;
            n /= 2;
        }
        node_cur = heap->root;
        while (level-- > 1)
        {
            /* TODO: Left and Right node could be packed into array */

            if (path & 1) node_cur = node_cur->right;
            else node_cur = node_cur->left;

            path >>= 1;
        }

        /* Insert the node */
        if (path & 1) { node_cur->right = new_node; }
        else { node_cur->left = new_node; }
        new_node->parent = node_cur;

        /* Tide the order */
        node_cur = new_node;
        while (node_cur->parent != NULL)
        {
            /* If node < parent then swap */
            if (heap->cb_cmp( \
                        &node_cur->key, \
                        &node_cur->parent->key) < 0)
            { break; }

            heap_swap_nodes(node_cur, node_cur->parent);
            node_cur = node_cur->parent;
        }
    }

    /* Update number of nodes */
    heap->size += 1;

    return 0;
}

heap_node_t *heap_root(heap_t *heap)
{
    return heap->root;
}

static void heap_remove_root_tidy(heap_t *heap, heap_node_t *node)
{
    /* If child < node then swap */
    if (node->left != NULL)
    {
        if (heap->cb_cmp(&node->left->key, &node->key) < 0)
        {
            heap_swap_nodes(node->left, node);
            heap_remove_root_tidy(heap, node->left);
        }

        if (node->right != NULL)
        {
            if (heap->cb_cmp(&node->right->key, &node->key) < 0)
            {
                heap_swap_nodes(node->right, node);
                heap_remove_root_tidy(heap, node->right);
            }
        }
    }
}

int heap_remove_root(heap_t *heap)
{
    heap_node_t *node_cur;
    qv_u32 path = 0, level = 0;
    qv_size_t n;

    if (heap->root == NULL) return -1;

    if (heap->size == 1)
    {
        /* Only one node as root */
        heap_node_destroy(heap, heap->root);
        heap->root = NULL;
    }
    else
    {
        /* Search the position at last level of heap */
        n = heap->size;
        while (n >= 2)
        {
            path = (path << 1) | (n & 1);

            level += 1;
            n /= 2;
        }
        node_cur = heap->root;
        while (level-- > 0)
        {
            /* TODO: Left and Right node could be packed into array */

            if (path & 1) node_cur = node_cur->right;
            else node_cur = node_cur->left;

            path >>= 1;
        }

        /* Move the final node to root */
        heap_move_nodes(heap->root, node_cur);
        if (node_cur->parent->left == node_cur)
        { node_cur->parent->left = NULL; }
        else
        { node_cur->parent->right = NULL; }

        /* Release the final node */
        heap_node_destroy(heap, node_cur);
        node_cur = NULL;

        /* Tide the order */
        heap_remove_root_tidy(heap, heap->root);
    }

    /* Update number of nodes */
    heap->size -= 1;

    return 0;
}

heap_node_t *heap_search_by(heap_t *heap, \
        heap_search_fn fn, \
        void *data)
{
    (void)heap;
    (void)fn;
    (void)data;
    return NULL;
}

int heap_remove(heap_t *heap, \
        heap_node_t *heap_node)
{
    (void)heap;
    (void)heap_node;
    return -1;
}

