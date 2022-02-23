/* Lightweight Service : Project File
 * Copyright(c) 2017-2018 y2c2 */

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "ujson.h"
#include "lws_project_file.h"

static lws_project_file_t *lws_project_file_new(void)
{
    lws_project_file_t *new_project_file;

    new_project_file = (lws_project_file_t *)malloc(sizeof(lws_project_file_t));
    if (new_project_file == NULL) { return NULL; }
    new_project_file->name = NULL;
    new_project_file->desc = NULL;
    new_project_file->version = NULL;

    return new_project_file;
}

void lws_project_file_destroy(lws_project_file_t *project_file)
{
    ec_delete(project_file->name);
    ec_delete(project_file->desc);
    ec_delete(project_file->version);

    free(project_file);
}

int lws_project_file_load_from_file( \
        lws_project_file_t **project_file_out, \
        const char *filename)
{
    lws_project_file_t *new_project_file = NULL;
    int ret = 0;
    char *data = NULL;
    size_t len;
    ujson_t *json = NULL;

    if (read_file(&data, &len, filename) != 0)
    {
        fprintf(stderr, "error: open file %s failed\n", filename);
        goto fail;
    }

    if ((json = ujson_parse(data, len)) == NULL)
    {
        fprintf(stderr, "error: corrupted data in %s, "
                "not valid JSON format\n", filename);
        goto fail;
    }

    if (ujson_type(json) != UJSON_OBJECT)
    {
        fprintf(stderr, "error: corrupted data in %s, "
                "not an object\n", filename);
        goto fail;
    }

    if ((new_project_file = lws_project_file_new()) == NULL)
    {
        fprintf(stderr, "error: out of memory\n");
        goto fail;
    }

    {
        ujson_t *json_deps;
        json_deps = ujson_as_object_lookup(json, "deps", 4);
        if (json_deps == NULL)
        {
            fprintf(stderr, "error: corrupted data in %s, "
                    "'deps' field not exists\n", filename);
            goto fail;
        }
        if (ujson_type(json_deps) != UJSON_ARRAY)
        {
            fprintf(stderr, "error: corrupted data in %s,"
                    "'deps' field not an array\n", filename);
            goto fail;
        }
        {
            ujson_object_item_t *item = ujson_as_object_first(json_deps);
            while (item != NULL)
            {
                ujson_t *u_value = ujson_as_object_item_value(item);
                if (ujson_type(u_value) != UJSON_OBJECT)
                {
                    fprintf(stderr, "error: corrupted data in %s, "
                            "'deps' item should be an object\n", filename);
                    goto fail;
                }
                item = ujson_as_object_next(item);
            }
        }
    }

    *project_file_out = new_project_file;
    new_project_file = NULL;

    goto done;
fail:
    ret = -1;
done:
    if (data != NULL) free(data);
    if (json != NULL) ujson_destroy(json);
    if (new_project_file != NULL) lws_project_file_destroy(new_project_file);
    return ret;
}

