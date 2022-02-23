/* Lightweight Service
 * Copyright(c) 2017-2018 y2c2 */

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ec_algorithm.h>
#include <ec_string.h>
#include <ec_encoding.h>
#include <ec_alloc.h>
#include "sourcelist.h"
#include "utils.h"
#include "qv.h"
#include "xjs.h"
#include "httpparse_allocator.h"
#include "ujson.h"
#include "lws_compile.h"
#include "lws_run.h"
#include "lws_subcmd.h"
#include "argsparse.h"

#define LWS_VERSION "0.0.1"

static void init_allocator(void)
{
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

    ec_allocator_set_malloc(malloc);
    ec_allocator_set_calloc(calloc);
    ec_allocator_set_free(free);
    ec_allocator_set_memset(memset);
    ec_allocator_set_memcpy(memcpy);

    xjs_allocator_set_malloc(malloc);
    xjs_allocator_set_calloc(calloc);
    xjs_allocator_set_free(free);
    xjs_allocator_set_memset(memset);
    xjs_allocator_set_memcpy(memcpy);

    hp_allocator_set_malloc(malloc);
    hp_allocator_set_free(free);

    ujson_allocator_set_malloc(malloc);
    ujson_allocator_set_free(free);
}

static void show_version(const char *exec_name)
{
    printf("Usage: %s " LWS_VERSION "\n", exec_name);
}

static void show_internal_commands(const char *exec_name)
{
    printf(""
        "Usage: %s [options] <script.js>\n"
        "\n"
        "  --list-sys-libs              List available system libraries\n"
        "  --ret-zero                   Return 0 when compilation failed\n"
        "\n"
        "", exec_name);
}

static void show_help(const char *exec_name)
{
    printf(""
        "Usage: %s [options]\n"
        "Usage: %s [options] <script.js>\n"
        "\n"
        "  init                         Initialize a new project\n"
        "\n"
        "  restore                      Restore packages of the project\n"
        "  run                          Run the project\n"
        "  build                        Build the project\n"
        "  test                         Test the project\n"
        "  clean                        Clean the output of build\n"
        "\n"
        "  add                          Add package to the project\n"
        "  remove                       Remove package from the project\n"
        "  list                         List all packages in the project\n"
        "\n"
        "  help                         Show help information\n"
        "\n"
        "  -h, --help                   Show help information\n"
        "  --internal-commands          Show internal commands\n"
        "  --version                    Show version information\n"
        "", exec_name, exec_name);
}

static void list_sys_libs(void)
{
    char **filenames = NULL;
    char buf_pwd[256];
    int filenames_count, i;
    int buf_pwd_len;

    memset(buf_pwd, 0, 256);
    if (current_working_path(buf_pwd, 256) != 0)
    {
        fprintf(stderr, "error: failed to list libraries\n");
        return;
    }
    buf_pwd_len = (int)strlen(buf_pwd);
    buf_pwd[buf_pwd_len] = OS_SEP;
    buf_pwd_len++;
    strcat(buf_pwd, "lib");

    if (list_dir(&filenames, &filenames_count, buf_pwd) != 0)
    {
        fprintf(stderr, "error: failed to list libraries\n");
        return;
    }

    for (i = 0; i < filenames_count; i++)
    {
        fputs(filenames[i], stdout);
        fputc('\n', stdout);

        free(filenames[i]);
    }
    free(filenames);
}

static int compile_and_run( \
        const sourcelist_ref sources, \
        const char *entry)
{
    int ret = 0;
    char *bytecode = NULL;
    size_t bytecode_len;

    if ((ret = lws_compile(&bytecode, &bytecode_len, sources, entry)) != 0)
    { goto fail; }

    if ((ret = lws_run(bytecode, bytecode_len)) != 0)
    { goto fail; }

    goto done;
fail:
    ret = -1;
done:
    if (bytecode != NULL) ec_free(bytecode);
    return ret;
}

typedef enum
{
    LWS_SUBCMD_EMPTY,
    LWS_SUBCMD_INIT,
    LWS_SUBCMD_RESTORE,
    LWS_SUBCMD_RUN,
    LWS_SUBCMD_BUILD,
    LWS_SUBCMD_TEST,
    LWS_SUBCMD_CLEAN,
    LWS_SUBCMD_ADD,
    LWS_SUBCMD_REMOVE,
    LWS_SUBCMD_LIST,
} lws_subcmd;

int main(int argc, char *argv[])
{
    int ret = 0;
    argsparse_t argsparse;
    char *entry = NULL, *entry_store = NULL;
    lws_filetype filetype = lws_filetype_auto;
    sourcelist_ref sources = NULL;
    int ret_zero = 0;
    lws_subcmd subcmd = LWS_SUBCMD_EMPTY;

#if defined(QV_PLATFORM_NT)
    qv_winsock_init();
#endif

    init_allocator();
    argsparse_init(&argsparse, argc, argv);
    if ((sources = ect_list_new(sourcelist)) == NULL)
    { fprintf(stderr, "error: out of memory\n"); ret = -1; goto fail; }

    if (argsparse_available(&argsparse) == 0)
    { fprintf(stderr, "error: no input file\n"); ret = -1; goto fail; }

    while (argsparse_available(&argsparse) != 0)
    {
        if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "init")))
        {
            subcmd = LWS_SUBCMD_INIT;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "restore")))
        {
            subcmd = LWS_SUBCMD_RESTORE;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "run")))
        {
            subcmd = LWS_SUBCMD_RUN;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "build")))
        {
            subcmd = LWS_SUBCMD_BUILD;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "test")))
        {
            subcmd = LWS_SUBCMD_TEST;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "clean")))
        {
            subcmd = LWS_SUBCMD_CLEAN;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "add")))
        {
            subcmd = LWS_SUBCMD_ADD;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "remove")))
        {
            subcmd = LWS_SUBCMD_REMOVE;
            argsparse_next(&argsparse);
        }
        else if ((argsparse.idx == 1) && (argsparse_match_str(&argsparse, "list")))
        {
            subcmd = LWS_SUBCMD_LIST;
            argsparse_next(&argsparse);
        }
        else
        {
            if (argsparse_match_str(&argsparse, "--help") || \
                    argsparse_match_str(&argsparse, "-h") || \
                    argsparse_match_str(&argsparse, "help"))
            { show_help(argv[0]); goto done; }
            else if (argsparse_match_str(&argsparse, "--internal-commands"))
            { show_internal_commands(argv[0]); goto done; }
            else if (argsparse_match_str(&argsparse, "--version"))
            { show_version(argv[0]); goto done; }
            else if (argsparse_match_str(&argsparse, "--list-sys-libs"))
            { list_sys_libs(); goto done; }
            else if (argsparse_match_str(&argsparse, "--ret-zero"))
            {
                argsparse_next(&argsparse);
                ret_zero = 1;
            }
            else
            {
                switch (subcmd)
                {
                    case LWS_SUBCMD_EMPTY:
                        {
                            const char *source_path = argsparse_fetch(&argsparse);
                            argsparse_next(&argsparse);
                            {
                                lws_source_ref new_lws_source;
                                if ((new_lws_source = lws_source_new(source_path, filetype)) == NULL)
                                { fprintf(stderr, "error: out of memory\n"); ret = -1; goto fail; }
                                ect_list_push_back(sourcelist, sources, new_lws_source);
                            }
                        }
                        break;

                    case LWS_SUBCMD_INIT:
                    case LWS_SUBCMD_RESTORE:
                    case LWS_SUBCMD_RUN:
                    case LWS_SUBCMD_BUILD:
                    case LWS_SUBCMD_TEST:
                    case LWS_SUBCMD_CLEAN:
                    case LWS_SUBCMD_ADD:
                    case LWS_SUBCMD_REMOVE:
                    case LWS_SUBCMD_LIST:
                        fprintf(stderr, "error: not implemented\n");
                        ret = -1;
                        goto fail;
                }
            }
        }
    }

    if (subcmd == LWS_SUBCMD_EMPTY)
    {
        if (ect_list_size(sourcelist, sources) == 0)
        { fprintf(stderr, "error: no source file\n"); ret = -1; goto fail; }

        {
            if (ect_list_size(sourcelist, sources) == 1)
            {
                const char *entry_filename = ect_list_front(sourcelist, sources)->source_path;
                const char *entry1;
                size_t entry1_len;
                if (mainname_get(&entry1, &entry1_len, entry_filename, strlen(entry_filename)) != 0)
                { fprintf(stderr, "error: invalid entry\n"); ret = -1; goto fail; }
                if ((entry_store = (char *)malloc(sizeof(char) * (entry1_len + 1))) == NULL)
                { fprintf(stderr, "error: out of memory\n"); ret = -1; goto fail; }
                memcpy(entry_store, entry1, entry1_len);
                entry_store[entry1_len] = '\0';
                entry = entry_store;
            }
            else
            { fprintf(stderr, "error: entry required\n"); ret = -1; goto fail; }
        }

        /* Compile & Run */
        if ((ret = compile_and_run(sources, entry)) != 0) { goto fail; }
    }
    else if (subcmd == LWS_SUBCMD_INIT)
    {
        lws_subcmd_init();
    }
    else if (subcmd == LWS_SUBCMD_RESTORE)
    {
        lws_subcmd_restore();
    }
    else if (subcmd == LWS_SUBCMD_BUILD)
    {
        lws_subcmd_build();
    }
    else
    { fprintf(stderr, "error: unsupported sub command\n"); ret = -1; goto fail; }

fail:
done:
    ec_delete(sources);
    if (ret_zero != 0) { ret = 0; }
    if (entry_store != NULL) free(entry_store);

#if defined(QV_PLATFORM_NT)
    qv_winsock_uninit();
#endif

    return ret;
}

