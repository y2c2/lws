/* qv : Watcher
 * Copyright(c) 2016 y2c2 */

#ifndef QV_WATCHER_H
#define QV_WATCHER_H

#include "qv_types.h"
#include "rbt.h"

struct qv_watcher
{
    /* Vector part */
    qv_handle_t **body;
    qv_size_t size;

    /* Tree part */
    rbt_t *events;

    qv_size_t used;
};

typedef struct qv_watcher qv_watcher_t;

int qv_watcher_init(qv_watcher_t *watcher);
void qv_watcher_uninit(qv_watcher_t *watcher);
int qv_watcher_set(qv_watcher_t *watcher, int fd, qv_handle_t *handle);
int qv_watcher_clr(qv_watcher_t *watcher, int fd);
qv_handle_t *qv_watcher_get(qv_watcher_t *watcher, int fd);


#endif

