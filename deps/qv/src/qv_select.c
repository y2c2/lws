/* qv : Backend Select
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"

#if defined(QV_PLATFORM_LINUX) || defined(QV_PLATFORM_FREEBSD)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#elif defined(QV_PLATFORM_MACOS)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#elif defined(QV_PLATFORM_NT)
#include <Winsock2.h>
#endif

#include "rbt.h"
#include "qv_allocator.h"
#include "qv_select.h"

struct qv_select
{
    rbt_t *fds;
};

typedef struct qv_select qv_select_t;


static int qv_fd_cmp_cb(rbt_node_data_t *key1, rbt_node_data_t *key2)
{
    if (key1->fd == key2->fd) return 0;
    return (key1->fd < key2->fd) ? -1 : 1;
}

int qv_select_init(void **backend_handle)
{
    int ret = 0;
    qv_select_t *new_select_handle = NULL;

    if ((new_select_handle = (qv_select_t *)qv_malloc(sizeof(qv_select_t))) == NULL)
    { return -1; }

    new_select_handle->fds = NULL;

    if ((new_select_handle->fds = rbt_new( \
            qv_malloc, qv_free, qv_fd_cmp_cb)) == NULL)
    {
        ret = -1;
        goto fail;
    }

    *backend_handle = new_select_handle;

    goto done;
fail:
    qv_select_uninit(backend_handle);
done:
    return ret;
}

int qv_select_uninit(void **backend_handle)
{
    qv_select_t *select_handle = (qv_select_t *)(*backend_handle);

    if (select_handle->fds != NULL)
    {
        rbt_destroy(select_handle->fds);
        select_handle->fds = NULL;
    }
    qv_free(select_handle);

    return 0;
}

int qv_select_ctl( \
        void *backend_handle, \
        int fd, \
        qv_select_op_t op,\
        qv_u32 triggered)
{
    qv_select_t *select_handle = (qv_select_t *)backend_handle;
    rbt_node_data_t key;
    rbt_node_data_t *value_to_search = NULL, value;

    /* Search if exists */
    key.fd = fd;
    rbt_search(select_handle->fds, &value_to_search, &key);

    switch (op)
    {
        case QV_SELECT_OP_ADD:
            if (value_to_search == NULL)
            {
                value.u32 = triggered;
                rbt_insert(select_handle->fds, &key, &value);
            }
            else
            {
                value_to_search->u32 |= triggered;
            }
            break;

        case QV_SELECT_OP_DEL:
            if (value_to_search == NULL)
            {
                /* There must be something wrong */
                return -1;
            }
            else
            {
                rbt_remove(select_handle->fds, &key);
            }
            break;

        case QV_SELECT_OP_MOD:
            if (value_to_search == NULL)
            {
                return -1;
            }
            else
            {
                value_to_search->u32 = triggered;
            }
            break;
    }

    return 0;
}

int qv_select_wait(void *backend_handle, int *fds, qv_u32 *events, int timeout)
{
    qv_select_t *select_handle = (qv_select_t *)backend_handle;
    struct timeval tv;
    rbt_iterator_t iter;
    int retval;
    qv_u32 event;
    int *fds_p = fds;
    qv_u32 *events_p = events;
    int fd;
    int nfds = 0;
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_exception;
    int fd_max = -1;

    /* Waiting timeout */
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    /* Clear & Fill */
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_exception);
    if (rbt_iterator_init(select_handle->fds, &iter) != 0)
    { return -1; }
    while (!rbt_iterator_isend(&iter))
    {
        fd = rbt_iterator_deref_key(&iter)->fd;
        event = rbt_iterator_deref_value(&iter)->u32;

        if (event & QV_SELECT_INPUT)
        { FD_SET(fd, &fd_read); }
        if (event & QV_SELECT_OUTPUT)
        { FD_SET(fd, &fd_write); }
        if (event & QV_SELECT_ERR)
        { FD_SET(fd, &fd_exception); }

        if (fd > fd_max) fd_max = fd;

        rbt_iterator_next(&iter);
    }

    /* Select */
    retval = select(fd_max + 1,\
            &fd_read, \
            &fd_write, \
            &fd_exception, \
            &tv);

    if (retval == -1)
    {
        return -1;
    }
    else if (retval)
    {
        if (rbt_iterator_init(select_handle->fds, &iter) != 0)
        { return -1; }
        while (!rbt_iterator_isend(&iter))
        {
            fd = rbt_iterator_deref_key(&iter)->fd;

            event = QV_SELECT_NONE;
            if (FD_ISSET(fd, &fd_read))
            { event |= QV_SELECT_INPUT; }
            if (FD_ISSET(fd, &fd_write))
            { event |= QV_SELECT_OUTPUT; }
            if (FD_ISSET(fd, &fd_exception))
            { event |= QV_SELECT_ERR; }

            if (event != QV_SELECT_NONE)
            {
                *fds_p++ = fd;
                *events_p++ = event;
                nfds++;
                if (nfds >= QV_WATCHER_MAXEVENTS)
                { return nfds; }
            }

            rbt_iterator_next(&iter);
        }
    }
    else
    {
        /* No data */
    }

    return nfds;
}

int qv_select_socket_tcp(void)
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int qv_select_socket_udp(void)
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

