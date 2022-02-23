/* Lightweight Service : Project File
 * Copyright(c) 2017-2018 y2c2 */

#ifndef LWS_PROJECT_FILE_H
#define LWS_PROJECT_FILE_H

#include <ec_algorithm.h>
#include <ec_string.h>
#include <ec_encoding.h>
#include <ec_alloc.h>

typedef enum
{
    lws_project_dep_type_local,
} lws_project_dep_type;

typedef struct
{
    ec_string *name;
    lws_project_dep_type type;
    union
    {
        struct
        {
            ec_string *url;
        } as_local;
    } u;

} lws_project_dep_t;

typedef struct
{
    ec_string *name;
    ec_string *version;
    ec_string *desc;

} lws_project_file_t;

int lws_project_file_load_from_file( \
        lws_project_file_t **project_file_out, \
        const char *filename);

void lws_project_file_destroy( \
        lws_project_file_t *project_file);

#endif

