/* qv : DNS
 * Copyright(c) 2016 y2c2 */

#include "qv_config.h"

#if defined(QV_PLATFORM_LINUX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#elif defined(QV_PLATFORM_NT)
#include <Winsock2.h>
#endif
#include "qv_allocator.h"
#include "qv_loop.h"
#include "qv_threadpool.h"
#include "qv_libc.h"
#include "qv_dns.h"

struct qv_getaddrinfo_stub
{
    char *node;
    char *service;
    struct addrinfo *hints;

    qv_handle_t *handle;
};
typedef struct qv_getaddrinfo_stub qv_getaddrinfo_stub_t;

static void qv_getaddrinfo_stub_destroy(qv_getaddrinfo_stub_t *stub);

static void qv_addrinfo_destroy(struct addrinfo *addrinfo)
{
    if (addrinfo->ai_next != NULL) qv_addrinfo_destroy(addrinfo->ai_next);
    if (addrinfo->ai_canonname != NULL) qv_free(addrinfo->ai_canonname);
    qv_free(addrinfo);
}

static int qv_addrinfo_dup(struct addrinfo **addr_info_out, const struct addrinfo *addrinfo)
{
    struct addrinfo *new_addrinfo;

    if (addrinfo == NULL)
    {
        *addr_info_out = NULL;
        return 0;
    } 

    if ((new_addrinfo = (struct addrinfo *)qv_malloc( \
                    sizeof(struct addrinfo))) == NULL)
    { return -1; }
    new_addrinfo->ai_flags = addrinfo->ai_flags;
    new_addrinfo->ai_family = addrinfo->ai_family;
    new_addrinfo->ai_socktype = addrinfo->ai_socktype;
    new_addrinfo->ai_protocol = addrinfo->ai_protocol;
    new_addrinfo->ai_addrlen = addrinfo->ai_addrlen;
    new_addrinfo->ai_addr = addrinfo->ai_addr;

    if (addrinfo->ai_canonname == NULL)
    {
        new_addrinfo->ai_canonname = NULL;
    }
    else
    {
        if ((new_addrinfo->ai_canonname = qv_strdup(addrinfo->ai_canonname)) == NULL)
        { goto fail; }
    }

    if (addrinfo->ai_next == NULL)
    {
        new_addrinfo->ai_next = NULL;
    }
    else
    {
        if (qv_addrinfo_dup(&new_addrinfo->ai_next, addrinfo->ai_next) != 0)
        { goto fail; }
    }

    *addr_info_out = new_addrinfo;

    return 0;
fail:
    if (new_addrinfo != NULL)
    { qv_addrinfo_destroy(new_addrinfo); }
    return -1;
}

static void qv_getaddrinfo_stub_destroy(qv_getaddrinfo_stub_t *stub)
{
    if (stub->node != NULL) { qv_free(stub->node); }
    if (stub->service != NULL) { qv_free(stub->service); }
    if (stub->hints != NULL) { qv_addrinfo_destroy(stub->hints); }
    qv_free(stub);
}

static qv_getaddrinfo_stub_t *qv_getaddrinfo_stub_new( \
        const char *node, \
        const char *service, \
        const struct addrinfo *hints, \
        qv_handle_t *handle)
{
    qv_getaddrinfo_stub_t *new_stub;

    if ((new_stub = (qv_getaddrinfo_stub_t *)qv_malloc( \
                    sizeof(qv_getaddrinfo_stub_t))) == NULL)
    { return NULL; }
    qv_memset(new_stub, 0, sizeof(qv_getaddrinfo_stub_t));
    new_stub->handle = handle;

    if (node != NULL)
    {
        if ((new_stub->node = qv_strdup(node)) == NULL)
        { goto fail; }
    }
    if (service != NULL)
    {
        if ((new_stub->service = qv_strdup(service)) == NULL)
        { goto fail; }
    }
    if (hints != NULL)
    {
        if ((qv_addrinfo_dup(&new_stub->hints, hints)) != 0)
        { goto fail; }
    }

    goto done;
fail:
done:
    return new_stub;
}

static void *qv_getaddrinfo_thread_routine(void *data)
{
    qv_getaddrinfo_stub_t *stub = (qv_getaddrinfo_stub_t *)data;
    qv_handle_t *handle = stub->handle;

    handle->u.getaddrinfo.ret = getaddrinfo( \
            stub->node, \
            stub->service, \
            stub->hints, \
            &handle->u.getaddrinfo.res);

    return NULL;
}

static void qv_getaddrinfo_thread_cont(void *data)
{
    qv_getaddrinfo_stub_t *stub = (qv_getaddrinfo_stub_t *)data;
    qv_handle_t *handle = stub->handle;
    int ret = handle->u.getaddrinfo.ret;
    struct addrinfo *response = (struct addrinfo *)handle->u.getaddrinfo.res;
    handle->u.getaddrinfo.getaddrinfo(handle, ret, response);

    /* Stub is useless now */
    qv_getaddrinfo_stub_destroy(stub);
}

int qv_getaddrinfo( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_getaddrinfo_cb getaddrinfo_cb, \
        const char *node, \
        const char *service, \
        const struct addrinfo* hints)
{
    qv_threadpool_t *threadpool = qv_loop_threadpool(loop);
    qv_getaddrinfo_stub_t *new_getaddrinfo_stub = NULL;

    if ((new_getaddrinfo_stub = qv_getaddrinfo_stub_new( \
                    node, service, hints, \
                    handle)) == NULL)
    { return -1; }

    if (qv_threadpool_dispatch( \
                threadpool, \
                qv_getaddrinfo_thread_routine, \
                new_getaddrinfo_stub, \
                qv_getaddrinfo_thread_cont) != 0)
    { goto fail; }

    handle->type = QV_HANDLE_TYPE_GETADDRINFO;
    handle->mode = QV_BACKEND_NONE;
    handle->loop = loop;
    handle->u.getaddrinfo.getaddrinfo = getaddrinfo_cb;
    handle->u.getaddrinfo.res = NULL;
    handle->u.getaddrinfo.ret = -1;

    return 0;
fail:
    if (new_getaddrinfo_stub != NULL)
    {
        qv_getaddrinfo_stub_destroy(new_getaddrinfo_stub);
    }
    return -1;
}

int qv_getnameinfo( \
        qv_loop_t *loop, \
        qv_handle_t *handle, \
        qv_getnameinfo_cb getnameinfo_cb, \
        const struct sockaddr *addr, \
        int flags)
{
    (void)loop;
    (void)handle;
    (void)getnameinfo_cb;
    (void)addr;
    (void)flags;

    return -1;
}

int qv_getaddrinfo_close(qv_handle_t *handle)
{

    if (handle->u.getaddrinfo.res != NULL)
    { freeaddrinfo(handle->u.getaddrinfo.res); }

    qv_free(handle);

    return 0;
}

int qv_getaddrinfo_process(qv_handle_t *handle)
{
    struct addrinfo *res = handle->u.getaddrinfo.res;

    handle->u.getaddrinfo.res = NULL;

    /* Get processed data */
    handle->u.getaddrinfo.getaddrinfo( \
            handle, \
            handle->u.getaddrinfo.ret, \
            res);

    return 0;
}

int qv_getnameinfo_process(qv_handle_t *handle)
{
    handle->u.getnameinfo.getnameinfo(handle, -1, NULL, NULL);
    return -1;
}

