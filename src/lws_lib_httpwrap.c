/* Lightweight Service : Library : HTTP Wrapper
 * Copyright(c) 2017-2018 y2c2 */

#include <stdlib.h>
#include <string.h>
#include "httpparse.h"
#include "qv.h"
#include "qv_libc.h"
#include "mbuf.h"
#include "lws_dist.h"
#include "lws_lib_httpwrap.h" 

/* HTTP status code */
typedef struct
{
    int code;
    char *desc;
} http_status_code_desc;

static http_status_code_desc http_status_code_desc_map[] = 
{
    { 100, "Continue" },
    { 101, "Switching Protocols" },
    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 203, "Non-Authoritative Information" },
    { 204, "No Content" },
    { 205, "Reset Content" },
    { 206, "Partial Content" },
    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 407, "Proxy Authentication Required" },
    { 408, "Request Time-out" },
    { 409, "Conflict" },
    { 410, "Gone" },
    { 411, "Length Required" },
    { 412, "Precondition Failed" },
    { 413, "Request Entity Too Large" },
    { 414, "Request-URI Too Large" },
    { 415, "Unsupported Media Type" },
    { 416, "Requested range not satisfiable" },
    { 417, "Expectation Failed" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Unavailable" },
    { 504, "Gateway Time-out" },
    { 505, "HTTP Version not supported" },
};

static char *http_status_code_desc_get(int status_code)
{
    int i;

    for (i = 0; i != sizeof(http_status_code_desc_map) / sizeof(http_status_code_desc); i++)
    {
        if (http_status_code_desc_map[i].code == status_code)
        {
            return http_status_code_desc_map[i].desc;
        }
    }

    return NULL;
}

typedef struct httpwrap_holder
{
    qv_handle_t handle;
    xjr_val v_server;
    xjr_vm *vm;
    xjr_mp_t *mp;
} httpwrap_holder_t;

/* Client */

typedef enum
{
    /* Not yet finished received a header */
    HTTPWRAP_CLIENT_STATE_HEADER,

    /* Received a header. now receiving a body */
    HTTPWRAP_CLIENT_STATE_BODY,
} httpwrap_client_state_t;

typedef struct httpwrap_client
{
    qv_handle_t handle;
    xjr_val v_server;
    xjr_vm *vm;
    xjr_mp_t *mp;

    httpwrap_client_state_t state;

    mbuf_t recv_buf;
} httpwrap_client_t;

static void *mbuf_malloc_cb(mbuf_size_t size)
{
    return malloc(size);
}

static void mbuf_free_cb(void *ptr)
{
    free(ptr);
}

static void *mbuf_memcpy_cb(void *dest, const void *src, mbuf_size_t n)
{
    return memcpy(dest, src, n);
}

static httpwrap_client_t *httpwrap_client_new(xjr_vm *vm, xjr_val v_server)
{
    lws_dist *dist = xjr_vm_get_distribution_data(vm);
    httpwrap_client_t *p;

    p = (httpwrap_client_t *)malloc(sizeof(httpwrap_client_t));
    if (p == NULL) return xjr_nullptr;
    qv_tcp_init(dist->loop, &p->handle);
    p->vm = vm;
    p->v_server = v_server;
    p->mp = vm->rts.rheap.mp;
    p->state = HTTPWRAP_CLIENT_STATE_HEADER;
    mbuf_init(&p->recv_buf, mbuf_malloc_cb, mbuf_free_cb, mbuf_memcpy_cb);
    qv_handle_set_data(&p->handle, p);
    return p;
}

static void httpwrap_client_destroy(httpwrap_client_t *hwc)
{
    free(hwc);
}

static void tcp_handle_turnoff(qv_handle_t *handle);

static httpwrap_holder_t *httpwrap_holder_new(xjr_vm *vm, xjr_val v_server)
{
    lws_dist *dist = xjr_vm_get_distribution_data(vm);
    httpwrap_holder_t *p = (httpwrap_holder_t *)malloc(sizeof(httpwrap_holder_t));
    if (p == NULL) return xjr_nullptr;
    qv_tcp_init(dist->loop, &p->handle);
    p->v_server = v_server;
    p->vm = vm;
    p->mp = vm->rts.rheap.mp;
    qv_handle_set_data(&p->handle, p);
    return p;
}

static void httpwrap_holder_destroy(httpwrap_holder_t *httpwrap)
{
    free(httpwrap);
}

static void *httpwrap_heap_malloc(void *heap, xjr_size_t size)
{
    (void)heap;
    return malloc(size);
}

static void httpwrap_heap_free(void *heap, void *p)
{
    (void)heap;
    free(p);
}

static void httpwrap_holder_recv_data_cb_on_error( \
        qv_handle_t *handle)
{
    httpwrap_client_t *hwc = qv_handle_get_data(handle);
    xjr_val v_server = hwc->v_server;
    xjr_vm *vm = hwc->vm;
    xjr_mp_t *mp = hwc->mp;
    xjr_val events;
    xjr_val_properties *props = xjr_val_as_object_property_get(mp, v_server);
    xjr_val_property_type prop_type;
    xjr_val callback;

    /* this._events */
    if (xjr_val_properties_get_by_name(mp, props, &events, &prop_type, "_events", 7) != 0) { return; }
    if (prop_type != XJR_VAL_PROPERTY_TYPE_NORMAL) { return; }
    if (!XJR_VAL_IS_OBJECT(events)) { return; }
    props = xjr_val_as_object_property_get(mp, events);

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
                httpwrap_heap_malloc, \
                httpwrap_heap_free, \
                xjr_nullptr, \
                callback, XJR_VAL_MAKE_UNDEFINED(), \
                1, XJR_VAL_MAKE_UNDEFINED()) != 0)
    {
        tcp_handle_turnoff(handle);
        return;
    }

    tcp_handle_turnoff(handle);
}

static void httpwrap_holder_recv_data_cb_on_request_sent( \
        qv_handle_t *handle, \
        int status)
{
    if (status < 0)
    {
        tcp_handle_turnoff(handle);
    }
}

static void httpwrap_holder_recv_data_cb_on_request( \
        qv_handle_t *handle, \
        httpparse_request_header_t *req_hdr, \
        const long int content_length)
{
    int retval = 0;
    httpwrap_client_t *hwc = qv_handle_get_data(handle);
    xjr_vm *vm = hwc->vm;
    xjr_mp_t *mp = hwc->mp;
    xjr_val callback;
    xjr_val incomingMessage;
    xjr_val serverResponse;

    xjr_size_t header_size = (xjr_size_t)req_hdr->data_len;

    /* handle.on('request', function (req: IncomingMessage, res: ServerResponse) {}) */
    callback = xjr_val_as_object_get_by_path(mp, hwc->v_server, \
            "_events", "request", xjr_nullptr);
    if (!XJR_VAL_IS_FUNCTION(callback))
    {
        /* No callback set but started receiving which indicates that
         * we should stop receiving data anymore */
        tcp_handle_turnoff(handle);
        goto finish;
    }

    /* req: IncomingMessage */
    /* incomingMessage = server._surface.createIncomingMessage(
     * httpVersion: String, 
     * method: String, 
     * url: String, 
     * headers: { key: String: value: String .. }
     * ) */
    {
        xjr_val v_httpversion = XJR_VAL_MAKE_UNDEFINED();
        xjr_val v_method = XJR_VAL_MAKE_UNDEFINED();
        xjr_val v_url = XJR_VAL_MAKE_UNDEFINED();
        xjr_val v_headers = xjr_val_make_object(mp);
        xjr_val v_body = XJR_VAL_MAKE_UNDEFINED();

        /* HTTP Version */
        switch (req_hdr->ver)
        {
            case HTTPPARSE_HTTP_VER_09:
            case HTTPPARSE_HTTP_VER_20:
                break;
            case HTTPPARSE_HTTP_VER_10:
                v_httpversion = xjr_val_make_string_from_heap(mp, "1.0", 3);
                break;
            case HTTPPARSE_HTTP_VER_11:
                v_httpversion = xjr_val_make_string_from_heap(mp, "1.1", 3);
                break;
        }

        /* Method */
        v_method = xjr_val_make_string_from_heap(mp, req_hdr->method, (xjr_size_t)req_hdr->method_len);

        /* URL */
        v_url = xjr_val_make_string_from_heap(mp, req_hdr->path, (xjr_size_t)req_hdr->path_len);

        /* Headers */
        {
            xjr_val_properties *props = xjr_val_as_object_property_get(mp, v_headers);
            hp_size_t i;
            for (i = 0; i != req_hdr->field_list.fields_count; i++)
            {
                xjr_val v_key, v_value;
                v_key = xjr_val_make_string_from_heap(mp, \
                        req_hdr->field_list.fields[i].key, 
                        (xjr_size_t)req_hdr->field_list.fields[i].key_len);
                v_value = xjr_val_make_string_from_heap(mp, \
                        req_hdr->field_list.fields[i].value, 
                        (xjr_size_t)req_hdr->field_list.fields[i].value_len);
                xjr_val_properties_set(mp, props, XJR_VAL_PROPERTY_TYPE_NORMAL, v_key, v_value);
            }
        }

        /* Body */
        v_body = xjr_val_make_string_from_heap(mp, \
                mbuf_body(&hwc->recv_buf) + req_hdr->data_len + 2, (xjr_size_t)content_length);

        {
            xjr_val createIncomingMessage = xjr_val_as_object_get_by_path(mp, hwc->v_server, \
                    "_surface", "createIncomingMessage", xjr_nullptr);
            if (!XJR_VAL_IS_FUNCTION(createIncomingMessage))
            {
                tcp_handle_turnoff(handle);
                goto finish;
            }
            {
                xjr_vm_register_external_val(vm, callback);
                retval = xjr_vm_call2(vm, 
                        NULL, httpwrap_heap_malloc, httpwrap_heap_free, \
                        &incomingMessage, \
                        createIncomingMessage /* callee */, \
                        XJR_VAL_MAKE_UNDEFINED() /* this */, \
                        5, \
                        v_httpversion, v_method, v_url, v_headers, v_body);
                xjr_vm_unregister_external_val(vm, callback);
            }
            if ((retval != 0) || (!XJR_VAL_IS_OBJECT(incomingMessage)))
            {
                tcp_handle_turnoff(handle);
                goto finish;
            }
        }
    }

    /* res: ServerResponse */
    /* serverResponse = server._surface.createServerResponse() */
    {
        xjr_val createServerResponse = xjr_val_as_object_get_by_path(mp, hwc->v_server, \
                "_surface", "createServerResponse", xjr_nullptr);
        if (!XJR_VAL_IS_FUNCTION(createServerResponse))
        {
            tcp_handle_turnoff(handle);
            goto finish;
        }
        {
            xjr_vm_register_external_val(vm, callback);
            xjr_vm_register_external_val(vm, incomingMessage);
            retval = xjr_vm_call2(vm, 
                    NULL, httpwrap_heap_malloc, httpwrap_heap_free, \
                    &serverResponse, \
                    createServerResponse /* callee */, \
                    XJR_VAL_MAKE_UNDEFINED() /* this */, \
                    0);
            xjr_vm_unregister_external_val(vm, callback);
            xjr_vm_unregister_external_val(vm, incomingMessage);
        }
        if ((retval != 0) || (!XJR_VAL_IS_OBJECT(serverResponse)))
        {
            tcp_handle_turnoff(handle);
            goto finish;
        }
    }

    /*
    {
        if (!XJR_VAL_IS_STRING(v_data))
        {
            tcp_handle_turnoff(handle);
            goto finish;
        }
    }
    */

    {
        xjr_vm_register_external_val(vm, callback);
        xjr_vm_register_external_val(vm, incomingMessage);
        xjr_vm_register_external_val(vm, serverResponse);
        retval = xjr_vm_call2(vm, \
                NULL, httpwrap_heap_malloc, httpwrap_heap_free, \
                xjr_nullptr, \
                callback /* callee */, \
                XJR_VAL_MAKE_UNDEFINED() /* this */, \
                2, incomingMessage, serverResponse);
        xjr_vm_unregister_external_val(vm, callback);
        xjr_vm_unregister_external_val(vm, incomingMessage);
        xjr_vm_unregister_external_val(vm, serverResponse);
    }
    if (retval != 0)
    {
        tcp_handle_turnoff(handle);
        goto finish;
    }

    {
        /* Extract the sendBuf of serverResponse */
        xjr_val sendBuf, statusCode, statusMsg;

        statusCode = xjr_val_as_object_get_by_path(mp, serverResponse, \
                "statusCode", xjr_nullptr);
        if (!XJR_VAL_IS_INTEGER(statusCode)) { tcp_handle_turnoff(handle); goto finish; }

        statusMsg = xjr_val_as_object_get_by_path(mp, serverResponse, \
                "statusMessage", xjr_nullptr);
        if (!XJR_VAL_IS_STRING(statusMsg)) { tcp_handle_turnoff(handle); goto finish; }

        sendBuf = xjr_val_as_object_get_by_path(mp, serverResponse, \
                "sendBuf", xjr_nullptr);
        if (!XJR_VAL_IS_STRING(sendBuf)) { tcp_handle_turnoff(handle); goto finish; }

        {
#define RESPONSE_BUF_SIZE 4096
            char *response_body = xjr_val_as_string_body(mp, sendBuf);
            int response_body_len = (int)xjr_val_as_string_length(mp, sendBuf);
            char response_buffer[RESPONSE_BUF_SIZE + 1];
            int response_data_len;
            int status_code_int = XJR_VAL_AS_INTEGER_UNTAG(statusCode);
            char *status_code_msg = NULL;

            if (xjr_val_as_string_length(mp, statusMsg) == 0)
            {
                status_code_msg = http_status_code_desc_get(status_code_int);
                if (status_code_msg == NULL)
                {
                    status_code_int = 500;
                    status_code_msg = http_status_code_desc_get(status_code_int);
                }
            }
            else
            {
                status_code_msg = xjr_val_as_string_body(mp, statusMsg);
            }

            response_data_len = (int)snprintf(response_buffer, 4096, \
                    "HTTP/1.1 %d %s\r\n"
                    "Content-Length: %u\r\n"
                    "\r\n", \
                    status_code_int, \
                    status_code_msg, \
                    (unsigned int)response_body_len);
            memcpy(response_buffer + response_data_len, response_body, (size_t)response_body_len);
            response_data_len += response_body_len;

            if (qv_tcp_send(handle, response_buffer, response_data_len, \
                        httpwrap_holder_recv_data_cb_on_request_sent) != 0)
            {
                tcp_handle_turnoff(handle);
                goto finish;
            }
        }
    }

finish:
    /* Shift out the header */
    mbuf_shift(&hwc->recv_buf, header_size);
    /* Shift out the body length */
    mbuf_shift(&hwc->recv_buf, (mbuf_size_t)content_length);
}

static void httpwrap_holder_recv_data_cb_on_data( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf)
{
    httpwrap_client_t *hwc = qv_handle_get_data(handle);

    /* New Data */

    /* Because of the HTTP is based on TCP stream, we append the received data in the buffer */
    mbuf_append(&hwc->recv_buf, buf, (mbuf_size_t)nread);

    switch (hwc->state)
    {
        case HTTPWRAP_CLIENT_STATE_HEADER:

            /* Try to decode a complete header in the stream
             * by finding "\r\n\r\n" */
            {
                const char *p_rnrn;
                p_rnrn = qv_strnstrn(mbuf_body(&hwc->recv_buf), mbuf_size(&hwc->recv_buf), \
                        "\r\n\r\n", 4);
                if (p_rnrn == NULL)
                {
                    /* Not yet received a complete header */
                    return;
                }
            }

            {
                httpparse_request_header_t req_hdr;
                httpparse_header_field_t *field;
                long int content_length = 0;

                /* Initialize Header Parser */
                httpparse_request_header_init(&req_hdr);

                if (httpparse_parse_request_header(&req_hdr, mbuf_body(&hwc->recv_buf), mbuf_size(&hwc->recv_buf)) != 0)
                {
                    /* Not a valid HTTP header */
                    mbuf_clear(&hwc->recv_buf);
                    tcp_handle_turnoff(handle);
                    return;
                }

                if ((field = httpparse_header_field_list_find(&req_hdr.field_list, "Content-Length", 14)) == NULL)
                {
                    content_length = 0;
                }
                else
                {
                    char *endptr;
                    content_length = strtol(field->value, &endptr, 10);
                    if ((field->value == endptr) || (content_length < 0))
                    {
                        mbuf_clear(&hwc->recv_buf);
                        httpparse_request_header_clear(&req_hdr);
                        tcp_handle_turnoff(handle);
                        return;
                    }
                }

                /* HTTP Version */
                if (req_hdr.ver != HTTPPARSE_HTTP_VER_11)
                {
                    /* TODO: Support more */
                    mbuf_clear(&hwc->recv_buf);
                    httpparse_request_header_clear(&req_hdr);
                    tcp_handle_turnoff(handle);
                    return;
                }

                if ((content_length == 0) || \
                        ((xjr_size_t)(content_length) + req_hdr.data_len + 4 > mbuf_size(&hwc->recv_buf)))
                {
                    httpwrap_holder_recv_data_cb_on_request(handle, &req_hdr, content_length);
                }

                httpparse_request_header_clear(&req_hdr);
            }
            break;

        case HTTPWRAP_CLIENT_STATE_BODY:

            /* TODO: Current not handle */
            mbuf_clear(&hwc->recv_buf);
            tcp_handle_turnoff(handle);
            break;
    }

}

static void httpwrap_holder_recv_data_cb( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf)
{
    if (nread <= 0)
    {
        httpwrap_holder_recv_data_cb_on_error(handle);
    }
    else
    {
        httpwrap_holder_recv_data_cb_on_data(handle, nread, buf);
    }
}

static void tcp_handle_turnoff(qv_handle_t *handle)
{
    httpwrap_holder_t *tw = qv_handle_get_data(handle);
    xjr_val v_server;
    xjr_vm *vm;
    xjr_mp_t *mp;

    if (tw == xjr_nullptr) return;

    v_server = tw->v_server;
    vm = tw->vm;
    mp = tw->mp;

    /* Call 'close' */
    {
        xjr_val dot_close = xjr_val_as_object_get_by_path(mp, tw->v_server, \
                "_events", "close", xjr_nullptr);
        if (XJR_VAL_IS_FUNCTION(dot_close))
        {
            if (xjr_vm_call2(vm, \
                        NULL, httpwrap_heap_malloc, httpwrap_heap_free, \
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
            xjr_val_as_object_extract(mp, v_server), \
            xjr_nullptr, xjr_nullptr, xjr_nullptr);
    xjr_vm_unregister_external_val(vm, v_server);
    qv_handle_set_data(&tw->handle, NULL);
    qv_tcp_recv_stop(&tw->handle);
    qv_tcp_close(&tw->handle);
    httpwrap_holder_destroy(tw);
}

/* Declarations */
static void httpwrap_holder_recv_data_cb( \
        qv_handle_t *handle, \
        qv_ssize_t nread, char *buf);

static void httpwrap_holder_listen_new_connection_cb( \
        qv_loop_t *loop, \
        qv_handle_t *server, \
        int status)
{
    httpwrap_holder_t *httpwrap_server = qv_handle_get_data(server);
    xjr_vm *vm = httpwrap_server->vm;
    xjr_val v_server = httpwrap_server->v_server;
    xjr_mp_t *mp = httpwrap_server->mp;
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

            if (xjr_vm_call2(httpwrap_server->vm, \
                    NULL, \
                    httpwrap_heap_malloc, \
                    httpwrap_heap_free, \
                    xjr_nullptr, \
                    callback, XJR_VAL_MAKE_UNDEFINED(), \
                    1, XJR_VAL_MAKE_UNDEFINED()) != 0)
            { return; }
        }
        else
        {
            httpwrap_client_t *new_httpwrap_client;

            new_httpwrap_client = httpwrap_client_new(vm, v_server); 
            if (new_httpwrap_client == NULL)
            {
                tcp_handle_turnoff(server);
                qv_loop_stop(loop);
                return;
            }

            /* Accept */
            if (qv_tcp_accept(server, &new_httpwrap_client->handle) != 0)
            {
                httpwrap_client_destroy(new_httpwrap_client);
                tcp_handle_turnoff(server);
                qv_loop_stop(loop);
                return;
            }

            /* Start receiving */
            qv_tcp_recv_start(&new_httpwrap_client->handle, httpwrap_holder_recv_data_cb);
        }
    }
}

static int httpwrap_holder_listen( \
        httpwrap_holder_t *httpwrap, \
        const char *address, const int port, const int backlog)
{
    qv_socketaddr_t addr;

    if (qv_socketaddr_init_ipv4(&addr, address, port) != 0)
    { return -1; }

    /* TODO: Resolve before binding */
    if (qv_tcp_bind(&httpwrap->handle, &addr) != 0)
    { return -1; }

    if (qv_tcp_listen(&httpwrap->handle, backlog, \
                httpwrap_holder_listen_new_connection_cb) != 0)
    { return -1; }

    return 0;
}

/* httpwrap.create(obj: Object) */
static void httpwrap_create(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    httpwrap_holder_t *httpwrap;
    xjr_val v_tcp;

    if (args->argc != 1) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);
    
    /* Clean exist httpwrap */
    httpwrap = xjr_val_object_get_attached_data(obj);
    if (httpwrap != xjr_nullptr)
    {
        httpwrap_holder_destroy(httpwrap);
        httpwrap = xjr_nullptr;
        xjr_val_object_set_attached_data(obj, xjr_nullptr, xjr_nullptr, xjr_nullptr);
    }

    if ((httpwrap = httpwrap_holder_new(args->vm, v_tcp)) == xjr_nullptr)
    {
        /* TODO: OOM Error */
        return;
    }
    xjr_val_object_set_attached_data(obj, httpwrap, xjr_nullptr, xjr_nullptr);

    /* Register external value */
    xjr_vm_register_external_val(args->vm, v_tcp);
}

/* httpwrap.listen(obj: Object, host: String, port: Number) */
static void httpwrap_listen(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    httpwrap_holder_t *httpwrap;
    if (args->argc != 4) return;
    if (!XJR_VAL_IS_STRING(args->argv[1])) return;
    if (!XJR_VAL_IS_NUMBER(args->argv[2])) return;
    obj = xjr_val_as_object_extract(args->mp, args->argv[0]);
    httpwrap = xjr_val_object_get_attached_data(obj);
    if (httpwrap == xjr_nullptr) return;
    if (httpwrap_holder_listen(httpwrap, \
                xjr_val_as_string_body(args->mp, args->argv[1]), \
                XJR_VAL_AS_INTEGER_UNTAG(args->argv[2]), \
                5) != 0)
    {
        args->ret = XJR_VAL_MAKE_INTEGER(-1);
    }
    else
    {
        args->ret = XJR_VAL_MAKE_INTEGER(0);
    }
}

/* httpwrap.close(obj: Object) */
static void httpwrap_close(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    httpwrap_holder_t *httpwrap;
    xjr_val v_http;

    if (args->argc != 1) return;

    v_http = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_http);
    httpwrap = xjr_val_object_get_attached_data(obj);
    tcp_handle_turnoff(&httpwrap->handle);
}

static void httpwrap_holder_sent_cb( \
        qv_handle_t *handle, \
        int status)
{
    if (status < 0) { tcp_handle_turnoff(handle); }
}

/* httpwrap.write(obj: Object, data: String) */
static void httpwrap_write(xjr_native_fn_args *args)
{
    xjr_val_object *obj;
    httpwrap_holder_t *httpwrap;
    xjr_val v_tcp, v_data;

    if (args->argc != 2) return;

    v_tcp = args->argv[0];
    obj = xjr_val_as_object_extract(args->mp, v_tcp);

    httpwrap = xjr_val_object_get_attached_data(obj);

    v_data = args->argv[1];
    if (XJR_VAL_IS_STRING(v_data))
    {
        char *body = xjr_val_as_string_body(args->mp, v_data);
        xjr_size_t len = xjr_val_as_string_length(args->mp, v_data);
        if (qv_tcp_send(&httpwrap->handle, body, len, httpwrap_holder_sent_cb) != 0)
        {
            tcp_handle_turnoff(&httpwrap->handle);
            return;
        }
    }
    else
    {
        return;
    }
}

int lws_lib_httpwrap(xjr_lib_ctx *ctx)
{
    xjr_lib_export_default()
    {
        xjr_lib_export_default_item("create", 6, xjr_val_make_native_function(ctx->mp, ctx->env, httpwrap_create));
        xjr_lib_export_default_item("listen", 6, xjr_val_make_native_function(ctx->mp, ctx->env, httpwrap_listen));
        xjr_lib_export_default_item("close", 5, xjr_val_make_native_function(ctx->mp, ctx->env, httpwrap_close));
        xjr_lib_export_default_item("write", 5, xjr_val_make_native_function(ctx->mp, ctx->env, httpwrap_write));
    }
    return 0;
}

