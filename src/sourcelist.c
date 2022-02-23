#include <ec_alloc.h>
#include "sourcelist.h"

static void xjs_source_ctor(void *data)
{
    lws_source_ref r = data;
    r->filetype = lws_filetype_auto;
    r->source_path = NULL;
}

static void xjs_source_dtor(void *data)
{
    lws_source_ref r = data;
    (void)r;
}

lws_source_ref lws_source_new(const char *source_path, const lws_filetype filetype)
{
    lws_source_ref r;
    if ((r = ec_newcd(lws_source, \
                    xjs_source_ctor, xjs_source_dtor)) == NULL) { return NULL; }
    r->source_path = source_path;
    r->filetype = filetype;
    return r;
}

static void sourcelist_node_dtor(lws_source *node)
{
    ec_delete(node);
}

ect_list_define_declared(sourcelist, lws_source *, sourcelist_node_dtor);

