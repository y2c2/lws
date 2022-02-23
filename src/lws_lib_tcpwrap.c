/* Lightweight Service : Library : TCP Wrapper
 * Copyright(c) 2017-2018 y2c2 */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include "qv.h"
#include "lws_dist.h"
#include "lws_lib_tcpwrap.h"

typedef struct tcpwrap_holder
{
    qv_handle_t handle;
    xjr_val obj;
    xjr_vm *vm;
    xjr_mp_t *mp;
} tcpwrap_holder_t;

/* Declarations */
static void tcpwrap_holder_recv_data_cb( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf);

static tcpwrap_holder_t *tcpwrap_holder_new(xjr_vm *vm, xjr_val obj)
{
    lws_dist *dist = xjr_vm_get_distribution_data(vm);
    tcpwrap_holder_t *p = (tcpwrap_holder_t *)malloc(sizeof(tcpwrap_holder_t));
    if (p == NULL) return xjr_nullptr;
    qv_tcp_init(dist->loop, &p->handle);
    p->obj = obj;
    p->vm = vm;
    p->mp = vm->rts.rheap.mp;
    qv_handle_set_data(&p->handle, p);
    return p;
}

static void tcpwrap_holder_destroy(tcpwrap_holder_t *tcpwrap)
{
    free(tcpwrap);
}

static void *tcpwrap_heap_malloc(void *heap, xjr_size_t size)
{
    (void)heap;
    return malloc(size);
}

static void tcpwrap_heap_free(void *heap, void *p)
{
    (void)heap;
    free(p);
}

static void tcp_handle_turnoff(qv_handle_t *handle)
{
    tcpwrap_holder_t *tw = qv_handle_get_data(handle);
    xjr_val v_tcp;
    xjr_vm *vm;
    xjr_mp_t *mp;

    if (tw == xjr_nullptr) return;

    v_tcp = tw->obj;
    vm = tw->vm;
    mp = tw->mp;

    /* Call 'close' */
    {
        xjr_val dot_close = xjr_val_as_object_get_by_path(mp, tw->obj, \
                "_events", "close", xjr_nullptr);
        if (XJR_VAL_IS_FUNCTION(dot_close))
        {
            if (xjr_vm_call2(vm, \
                        NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                        xjr_nullptr, \
                        dot_close, \
                        XJR_VAL_MAKE_UNDEFINED() /* this */, \
                        0) != 0)
            {
                /* Whatever */ 
            }
        }
    }

    xjr_val_object_set_attached_data( \
            xjr_val_as_object_extract(mp, v_tcp), \
            xjr_nullptr, xjr_nullptr, xjr_nullptr);
    xjr_vm_unregister_external_val(vm, v_tcp);
    qv_handle_set_data(&tw->handle, NULL);
    qv_tcp_recv_stop(&tw->handle);
    qv_tcp_close(&tw->handle);
    tcpwrap_holder_destroy(tw);
}

static void tcpwrap_holder_listen_new_connection_cb( \
        qv_loop_t *loop, \
        qv_handle_t *server, \
        int status)
{
    tcpwrap_holder_t *tcpwrap_server = qv_handle_get_data(server);
    xjr_vm *vm = tcpwrap_server->vm;
    xjr_val v_server = tcpwrap_server->obj;
    xjr_mp_t *mp = tcpwrap_server->mp;
    xjr_val events;
    xjr_val_properties *props = xjr_val_as_object_property_get(mp, v_server);
    xjr_val_property_type prop_type;

    /* this._events */
    if (xjr_val_properties_get_by_name(mp, props, &events, &prop_type, "_events", 7) != 0) { return; }
    if (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL) { return; }
    if (!XJR_VAL_IS_OBJECT(events)) { return; }
    props = xjr_val_as_object_property_get(mp, events);

    {
        xjr_val callback;

        if (status < 0)
        {
            /* Error */

            /* server.on('error', function (e) {}) */
            if (xjr_val_properties_get_by_name(mp, props, &callback, &prop_type, "error", 5) != 0)
            { return; }
            if (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL) { return; }

            if (xjr_vm_call2(tcpwrap_server->vm, \
                    NULL, \
                    tcpwrap_heap_malloc, \
                    tcpwrap_heap_free, \
                    xjr_nullptr, \
                    callback, XJR_VAL_MAKE_UNDEFINED(), \
                    1, XJR_VAL_MAKE_UNDEFINED()) != 0)
            { return; }
        }
        else
        {
            /* New connection */
            xjr_val client;
            tcpwrap_holder_t *tcpwrap_client = xjr_nullptr;

            /* client = server._surface.createSocket() */
            xjr_val createSocket = xjr_val_as_object_get_by_path(mp, v_server, \
                    "_surface", "createSocket", xjr_nullptr);
            if (!XJR_VAL_IS_FUNCTION(createSocket))
            { return; }
            if ((xjr_vm_call2(vm, \
                            NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                            &client, \
                            createSocket /* callee */, \
                            XJR_VAL_MAKE_UNDEFINED() /* this */, \
                            0) != 0) || \
                    (!XJR_VAL_IS_OBJECT(client)))
            {
                tcp_handle_turnoff(server);
                qv_loop_stop(loop);
                return;
            }

            /* Accept */
            if ((tcpwrap_client = tcpwrap_holder_new(vm, client)) == xjr_nullptr)
            { return; }
            qv_tcp_accept(server, &tcpwrap_client->handle);
            xjr_val_object_set_attached_data(xjr_val_as_object_extract(mp, client), \
                    tcpwrap_client, xjr_nullptr, xjr_nullptr);

            /* server.on('connection', function (client) {}) */
            if (xjr_val_properties_get_by_name(mp, props, &callback, &prop_type, "connection", 10) != 0)
            { return; }
            if (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL) { return; }

            /* Register the value */
            xjr_vm_register_external_val(vm, client);

            if (xjr_vm_call2(vm, \
                    NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                    xjr_nullptr, \
                    callback /* callee */, \
                    XJR_VAL_MAKE_UNDEFINED() /* this */, \
                    1, client) != 0)
            { return; }

            /* If '_events.data' available, start receiving */
            if (!XJR_VAL_IS_UNDEFINED(xjr_val_as_object_get_by_path(mp, tcpwrap_client->obj, \
                            "_events", "data", xjr_nullptr)))
            {
                qv_tcp_recv_start(&tcpwrap_client->handle, tcpwrap_holder_recv_data_cb);
            }
        }
    }
}

static int tcpwrap_holder_listen( \
        tcpwrap_holder_t *tcpwrap, \
        const char *address, const int port, const int backlog)
{
    qv_socketaddr_t addr;

    if (qv_socketaddr_init_ipv4(&addr, address, port) != 0)
    { return -1; }

    /* TODO: Resolve before binding */
    if (qv_tcp_bind(&tcpwrap->handle, &addr) != 0)
    { return -1; }

    if (qv_tcp_listen(&tcpwrap->handle, backlog, tcpwrap_holder_listen_new_connection_cb) != 0)
    { return -1; }

    return 0;
}

static void tcpwrap_holder_connect_on_connected(qv_loop_t *loop, qv_handle_t *handle, int status)
{
    tcpwrap_holder_t *tw = qv_handle_get_data(handle);
    xjr_vm *vm = tw->vm;
    xjr_mp_t *mp = tw->mp;

    (void)loop;

    if (status < 0)
    {
        /* Error */
        xjr_val dot_error = xjr_val_as_object_get_by_path(mp, tw->obj, \
                "_events", "error", xjr_nullptr);
        if (!XJR_VAL_IS_FUNCTION(dot_error))
        {
            tcp_handle_turnoff(handle);
            return;
        }
        if (xjr_vm_call2(vm, \
                    NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                    xjr_nullptr, \
                    dot_error, \
                    XJR_VAL_MAKE_UNDEFINED() /* this */, \
                    0) != 0)
        {
            tcp_handle_turnoff(handle);
            return;
        }

        tcp_handle_turnoff(handle);
    }
    else
    {
        /* handle._events.conenct */
        xjr_val dot_connect = xjr_val_as_object_get_by_path(mp, tw->obj, \
                "_events", "connect", xjr_nullptr);
        if (!XJR_VAL_IS_FUNCTION(dot_connect)) { return; }
        if (xjr_vm_call2(vm, \
                    NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                    xjr_nullptr, \
                    dot_connect, \
                    XJR_VAL_MAKE_UNDEFINED() /* this */, \
                    0) != 0)
        {
            tcp_handle_turnoff(handle);
            return;
        }

        /* If '_events.data' available, start receiving */
        if (!XJR_VAL_IS_UNDEFINED(xjr_val_as_object_get_by_path(mp, tw->obj, \
                        "_events", "data", xjr_nullptr)))
        {
            qv_tcp_recv_start(handle, tcpwrap_holder_recv_data_cb);
        }
    }
}

static int tcpwrap_holder_connect( \
        tcpwrap_holder_t *tcpwrap, \
        const char *address, const int port)
{
    qv_socketaddr_t addr;

    if (qv_socketaddr_init_ipv4(&addr, address, port) != 0)
    { return -1; }

    /* TODO: Resolve before connecting */
    if (qv_tcp_connect(&tcpwrap->handle, &addr, tcpwrap_holder_connect_on_connected) != 0)
    {
        /*
        int errno1 = errno;
        switch (errno1)
        {
            case EBADF:
                printf("EBADF\n");
                break;
            case EEXIST:
                printf("EEXIST\n");
                break;
            case EINVAL:
                printf("EINVAL\n");
                break;
            case ELOOP:
                printf("ELOOP\n");
                break;
            case ENOENT:
                printf("ENOENT\n");
                break;
            case ENOMEM:
                printf("ENOMEM\n");
                break;
            case ENOSPC:
                printf("ENOSPC\n");
                break;
            case EPERM:
                printf("EPERM\n");
                break;
        }
        */

        return -1;
    }
    return 0;
}

/* tcpwrap.create(obj: Object) */
static void tcpwrap_create(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tcpwrap;
    xjr_val v_tcp;

    if (args->argc != 1) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);
    
    /* Clean exist tcpwrap */
    tcpwrap = xjr_val_object_get_attached_data(obj);
    if (tcpwrap != xjr_nullptr)
    {
        tcpwrap_holder_destroy(tcpwrap);
        tcpwrap = xjr_nullptr;
        xjr_val_object_set_attached_data(obj, xjr_nullptr, xjr_nullptr, xjr_nullptr);
    }

    if ((tcpwrap = tcpwrap_holder_new(args->vm, v_tcp)) == xjr_nullptr)
    {
        /* TODO: OOM Error */
        return;
    }
    xjr_val_object_set_attached_data(obj, tcpwrap, xjr_nullptr, xjr_nullptr);

    /* Register external value */
    xjr_vm_register_external_val(args->vm, v_tcp);
}

/* tcpwrap.listen(obj: Object, host: String, port: Number, backlog: Number) */
static void tcpwrap_listen(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tcpwrap;
    if (args->argc != 4) return;
    if (!XJR_VAL_IS_STRING(args->argv[1])) return;
    if (!XJR_VAL_IS_NUMBER(args->argv[2])) return;
    if (!XJR_VAL_IS_NUMBER(args->argv[3])) return;
    obj = xjr_val_as_object_extract(args->mp, args->argv[0]);
    tcpwrap = xjr_val_object_get_attached_data(obj);
    if (tcpwrap == xjr_nullptr) return;
    if (tcpwrap_holder_listen(tcpwrap, \
                xjr_val_as_string_body(args->mp, args->argv[1]), \
                XJR_VAL_AS_INTEGER_UNTAG(args->argv[2]), \
                XJR_VAL_AS_INTEGER_UNTAG(args->argv[3])) != 0)
    {
        args->ret = XJR_VAL_MAKE_INTEGER(-1);
    }
    else
    {
        args->ret = XJR_VAL_MAKE_INTEGER(0);
    }
}

/* tcpwrap.connect(obj: Object, host: String, port: Number) */
static void tcpwrap_connect(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tw;
    if (args->argc != 3) return;
    if (!XJR_VAL_IS_STRING(args->argv[1])) return;
    if (!XJR_VAL_IS_NUMBER(args->argv[2])) return;
    obj = xjr_val_as_object_extract(args->mp, args->argv[0]);
    tw = xjr_val_object_get_attached_data(obj);
    if (tw == xjr_nullptr) return;
    if (tcpwrap_holder_connect(tw, \
                xjr_val_as_string_body(args->mp, args->argv[1]), \
                XJR_VAL_AS_INTEGER_UNTAG(args->argv[2])) != 0)
    {
        args->ret = XJR_VAL_MAKE_INTEGER(-1);
    }
    else
    {
        args->ret = XJR_VAL_MAKE_INTEGER(0);
    }
}

/* tcpwrap.close(obj: Object) */
static void tcpwrap_close(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tcpwrap;
    xjr_val v_tcp;

    if (args->argc != 1) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);
    tcpwrap = xjr_val_object_get_attached_data(obj);
    tcp_handle_turnoff(&tcpwrap->handle);
}

static void tcpwrap_holder_sent_cb( \
        qv_handle_t *handle, \
        int status)
{
    if (status < 0) { tcp_handle_turnoff(handle); }
}

/* tcpwrap.write(obj: Object, data: String) */
static void tcpwrap_write(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tcpwrap;
    xjr_val v_tcp, v_data;

    if (args->argc != 2) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);

    tcpwrap = xjr_val_object_get_attached_data(obj);

    v_data = args->argv[1];
    if (XJR_VAL_IS_STRING(v_data))
    {
        char *body = xjr_val_as_string_body(args->mp, v_data);
        xjr_size_t len = xjr_val_as_string_length(args->mp, v_data);
        if (qv_tcp_send(&tcpwrap->handle, body, len, tcpwrap_holder_sent_cb) != 0)
        {
            tcp_handle_turnoff(&tcpwrap->handle);
            return;
        }
    }
    else
    {
        return;
    }
}

static void tcpwrap_holder_recv_data_cb( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf)
{
    tcpwrap_holder_t *tcpwrap = qv_handle_get_data(handle);
    xjr_vm *vm = tcpwrap->vm;
    xjr_val v_server = tcpwrap->obj;
    xjr_mp_t *mp = tcpwrap->mp;
    xjr_val events;
    xjr_val_properties *props = xjr_val_as_object_property_get(mp, v_server);
    xjr_val_property_type prop_type;

    /* this._events */
    if (xjr_val_properties_get_by_name(mp, props, &events, &prop_type, "_events", 7) != 0) { return; }
    if (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL) { return; }
    if (!XJR_VAL_IS_OBJECT(events)) { return; }
    props = xjr_val_as_object_property_get(mp, events);

    {
        xjr_val callback;
        if (nread <= 0)
        {
            /* Error */

            /* handle.on('error', function (e) {}) */
            if ((xjr_val_properties_get_by_name(mp, props, &callback, &prop_type, "error", 5) != 0) ||
                    (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL))
            {
                /* No 'error' callback set */
                tcp_handle_turnoff(handle);
                return;
            }

            if (xjr_vm_call2(vm, \
                    NULL, \
                    tcpwrap_heap_malloc, \
                    tcpwrap_heap_free, \
                    xjr_nullptr, \
                    callback, XJR_VAL_MAKE_UNDEFINED(), \
                    1, XJR_VAL_MAKE_UNDEFINED()) != 0)
            {
                tcp_handle_turnoff(handle);
                return;
            }

            tcp_handle_turnoff(handle);
        }
        else
        {
            /* New Data */

            /* handle.on('data', function (data) {}) */
            if ((xjr_val_properties_get_by_name(mp, props, &callback, &prop_type, "data", 4) != 0) && \
                    (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL))
            {
                /* No callback set but started receiving which indicates that
                 * we should stop receiving data anymore */
                qv_tcp_recv_stop(handle);
                return;
            }

            xjr_val v_data = xjr_val_make_string_from_heap(mp, buf, (xjr_size_t)nread);
            if (!XJR_VAL_IS_STRING(v_data))
            {
                /* Out of memory */
                qv_tcp_recv_stop(handle);
                return;
            }

            {
                int ret;
                xjr_vm_register_external_val(vm, v_data);
                ret = xjr_vm_call2(vm, \
                        NULL, tcpwrap_heap_malloc, tcpwrap_heap_free, \
                        xjr_nullptr, \
                        callback /* callee */, \
                        XJR_VAL_MAKE_UNDEFINED() /* this */, \
                        1, v_data);
                xjr_vm_unregister_external_val(vm, v_data);
                if (ret != 0)
                {
                    tcp_handle_turnoff(handle);
                    return;
                }
            }
        }
    }
}

/* tcpwrap.recvStart(obj: Object) */
static void tcpwrap_recvstart(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    tcpwrap_holder_t *tcpwrap;
    xjr_val v_tcp;

    if (args->argc != 1) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);

    tcpwrap = xjr_val_object_get_attached_data(obj);
    if (tcpwrap != xjr_nullptr)
    {
        qv_tcp_recv_start(&tcpwrap->handle, tcpwrap_holder_recv_data_cb);
    }
}

/*
 * export default {
 *   "create": tcpwrap_create,
 *   "listen": tcpwrap_listen
 *   "connect": tcpwrap_connect
 *   "close": tcpwrap_close
 *   "write": tcpwrap_write
 *   "recvStart": tcpwrap_recvstart
 * };
 */

int lws_lib_tcpwrap(xjr_lib_ctx *ctx)
{
    xjr_lib_export_default()
    {
        xjr_lib_export_default_item("create", 6, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_create));
        xjr_lib_export_default_item("listen", 6, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_listen));
        xjr_lib_export_default_item("connect", 7, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_connect));
        xjr_lib_export_default_item("close", 5, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_close));
        xjr_lib_export_default_item("write", 5, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_write));
        xjr_lib_export_default_item("recvStart", 9, xjr_val_make_native_function(ctx->mp, ctx->env, tcpwrap_recvstart));
    }
    return 0;
}

