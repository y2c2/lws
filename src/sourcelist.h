#ifndef SOURCELIST_H
#define SOURCELIST_H

#include <ec_string.h>
#include <ec_list.h>

typedef enum
{
    lws_filetype_auto,
    lws_filetype_js,
    lws_filetype_mjs,
    lws_filetype_x,
} lws_filetype;

typedef struct
{
    const char *source_path;
    lws_filetype filetype;
} lws_source;

typedef lws_source *lws_source_ref;

lws_source_ref lws_source_new(const char *source_path, const lws_filetype filetype);

ect_list_declare(sourcelist, lws_source *);
typedef sourcelist *sourcelist_ref;

#endif
