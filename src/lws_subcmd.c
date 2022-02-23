/* Lightweight Service : Sub Commands
 * Copyright(c) 2017-2018 y2c2 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "ujson.h"
#include "lws_project_file.h"
#include "lws_subcmd.h"

#define PROJECT_FILENAME "project.json"
#define PROJECT_NAME_MAX 64
#define VERSION_MAX 64
#define DESC_MAX 128

#define VERSION_DEFAULT "1.0.0" 

/*
 * project.json
 *
 * {
 *   "name": "<DIRNAME>",
 *   "version": "<VERSION>",
 *   "desc": "<DESC>"
 *   "deps": [
 *     {
 *       "name": "webframework",
 *       "type": "local",
 *       "url": "./packages/webframework",
 *     },
 *     { "name": "sqlite" }
 *   ]
 * }
 */
static void lws_subcmd_init_opt( \
        const char *name, \
        const char *version, \
        const char *desc)
{
    FILE *fp = NULL;

    /* Skip if the project file exists */
    if ((fp = fopen(PROJECT_FILENAME, "rb")) != NULL)
    {
        fprintf(stderr, "warning: " PROJECT_FILENAME " exists\n");
        fclose(fp);
        return;
    }

    if ((fp = fopen(PROJECT_FILENAME, "wb+")) == NULL)
    {
        fprintf(stderr, "error: failed to create " PROJECT_FILENAME "\n");
        return;
    }

    fprintf(fp, ""
        "{\n"
        "  \"name\": \"%s\",\n"
        "  \"version\": \"%s\",\n"
        "  \"desc\": \"%s\",\n"
        "  \"deps\": [\n"
        "  ]\n"
        "}", \
        name, \
        version, \
        desc);

    fclose(fp);
}

void lws_subcmd_init(void)
{
    char project_name[PROJECT_NAME_MAX + 1];
    char project_name_entered[PROJECT_NAME_MAX + 1];
    char *project_name_final = NULL;

    char version[VERSION_MAX + 1];
    char version_entered[VERSION_MAX + 1];
    char *version_final = NULL;

    char desc[DESC_MAX + 1];
    char desc_entered[DESC_MAX + 1];
    char *desc_final = NULL;
    int len;

    /* Project name */
    {
        char cwd[PATH_MAX + 1];
        const char *basename_p;
        size_t basename_len;
        size_t cwd_len;

        if (current_working_path(cwd, PATH_MAX) != 0)
        {
            fprintf(stderr, "error: get current working directory failed\n");
            return;
        }

        cwd_len = strlen(cwd);

        if (basename_get( \
                    &basename_p, \
                    &basename_len, \
                    cwd, cwd_len) != 0)
        {
            fprintf(stderr, "error: get basename failed\n");
            return;
        }
        memcpy(project_name, basename_p, basename_len);
        project_name[basename_len] = '\0';

        if (strlen(project_name) != 0)
        {
            printf("project name: (%s) ", project_name);
        }
        else
        {
            printf("project name: ");
        }

        fgets(project_name_entered, PROJECT_NAME_MAX, stdin);
        len = (int)strlen(project_name_entered);
        if ((len > 0) && (project_name_entered[len - 1] == '\n'))
        {
            project_name_entered[len - 1] = '\0';
        }

        if (strlen(project_name_entered) != 0)
        {
            project_name_final = project_name_entered;
        }
        else
        {
            project_name_final = project_name;
        }
    }

    /* Version */
    {
        snprintf(version, VERSION_MAX, VERSION_DEFAULT);

        printf("version: (%s) ", version);

        fgets(version_entered, VERSION_MAX, stdin);
        len = (int)strlen(version_entered);
        if ((len > 0) && (version_entered[len - 1] == '\n'))
        {
            version_entered[len - 1] = '\0';
        }

        if (strlen(version) != 0)
        {
            version_final = version_entered;
        }
        else
        {
            version_final = version;
        }
    }

    /* Desc */
    {
        desc[0] = '\0';

        printf("description: ");

        fgets(desc_entered, DESC_MAX, stdin);
        len = (int)strlen(desc_entered);
        if ((len > 0) && (desc_entered[len - 1] == '\n'))
        {
            desc_entered[len - 1] = '\0';
        }

        if (strlen(desc) != 0)
        {
            desc_final = desc_entered;
        }
        else
        {
            desc_final = desc;
        }
    }

    lws_subcmd_init_opt( \
            project_name_final,
            version_final,
            desc_final);
}

void lws_subcmd_restore(void)
{
}

void lws_subcmd_build(void)
{
}

