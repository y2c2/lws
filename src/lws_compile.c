#include <string.h>
#include <stdlib.h>
#include <ec_algorithm.h>
#include <ec_string.h>
#include <ec_encoding.h>
#include <ec_alloc.h>
#include "print_error.h"
#include "xjs.h"
#include "sourcelist.h"
#include "utils.h"
#include "lws_compile.h"

#define LWS_LIB_NAME "lib" 
#define LWS_STDLIB_FILENAME "stdlib.js"

#ifdef PATH_MAX
#define XJSCD_PATH_MAX PATH_MAX
#else
#define XJSCD_PATH_MAX 255
#endif

static int xjs_load_sys_lib_cb( \
        char **data_out, xjs_size_t *size_out, \
        char **lib_fullpath_out, xjs_size_t *lib_fullpath_len_out, \
        const char *name, const xjs_size_t name_len, \
        xjs_u32 opts)
{
    int ret = 0;
    char exe_path[XJSCD_PATH_MAX];
    char lib_path[XJSCD_PATH_MAX];
    char *p;
    char *source_data = NULL;
    size_t source_len = 0;
    char *lib_fullpath = NULL;
    xjs_size_t lib_fullpath_len = 0;

    (void)opts;
    (void)name_len;

    if (current_program_path(exe_path, XJSCD_PATH_MAX) != 0) { goto fail; }
    p = strrchr(exe_path, OS_SEP);
    if (p == NULL) { goto fail; }
    *p = '\0';
    lib_fullpath_len = (xjs_size_t)snprintf(lib_path, XJSCD_PATH_MAX, "%s%clib%c%s.mjs", exe_path, OS_SEP, OS_SEP, name);

    if ((lib_fullpath = (char *)ec_malloc(sizeof(char) * (lib_fullpath_len + 1))) == NULL)
    { goto fail; }
    memcpy(lib_fullpath, lib_path, lib_fullpath_len);
    lib_fullpath[lib_fullpath_len] = '\0';

    if (read_file(&source_data, &source_len, lib_path) != 0) { goto fail; }

    *data_out = source_data;
    *size_out = source_len;
    *lib_fullpath_out = lib_fullpath;
    *lib_fullpath_len_out = lib_fullpath_len;

    goto done;
fail:
    ret = -1;
    if (lib_fullpath != NULL) ec_free(lib_fullpath);
done:
    return ret;
}

int lws_compile( \
        char **bytecode_out, size_t *bytecode_len_out, \
        const sourcelist_ref sources, \
        const char *entry)
{
    int ret = 0;

    xjs_ir_ref *irs = NULL;
    xjs_ir_ref ir_merged = NULL;
    ect_iterator(sourcelist) it_src;
    ec_size_t srcs_count = 0, i;
    xjs_error err;
    const char *source_first = NULL;
    xjs_ir_ref new_ir = NULL;
    char *bytecode = NULL;
    xjs_size_t bytecode_len;

    xjs_error_init(&err);

    srcs_count = ect_list_size(sourcelist, sources);
    if ((irs = (xjs_ir_ref *)malloc(sizeof(xjs_ir_ref) * srcs_count)) == NULL)
    { fprintf(stderr, "error: out of memory\n"); return -1; }
    for (i = 0; i != srcs_count; i++) { irs[i] = NULL; }

    i = 0;
    ect_for(sourcelist, sources, it_src)
    {
        ec_string *uscript = NULL;
        char *source_data = NULL;
        size_t source_len;
        xjs_token_list_ref tokens = NULL;
        xjs_ast_program_ref ast = NULL;
        xjs_cfg_ref cfg = NULL;
        lws_source_ref source = ect_deref(lws_source_ref, it_src);

        if (source_first == NULL) source_first = source->source_path;

        /* Read source file */
        if (read_file(&source_data, &source_len, source->source_path) != 0)
        {
            fprintf(stderr, "error: failed to open file %s\n", source->source_path);
            goto fail;
        }

        if (source->filetype == lws_filetype_auto)
        {
            /* Which format? */
            const char *p_end = source->source_path + strlen(source->source_path);
            const char *p_dot = strrchr(source->source_path, '.');
            if (p_dot == NULL)
            {
                fprintf(stderr, "error: file not recognized %s\n", source->source_path);
                free(source_data);
                goto fail;
            }
            if ((p_end - p_dot >= 4) && (strncmp(p_dot, ".mjs", 4) == 0))
            {
                source->filetype = lws_filetype_mjs;
            }
            else if ((p_end - p_dot >= 3) && (strncmp(p_dot, ".js", 3) == 0))
            {
                source->filetype = lws_filetype_js;
            }
            else if ((p_end - p_dot >= 2) && (strncmp(p_dot, ".x", 2) == 0))
            {
                source->filetype = lws_filetype_x;
            }
            else
            {
                fprintf(stderr, "error: file not recognized %s\n", source->source_path);
                free(source_data);
                goto fail;
            }
        }

        if ((source->filetype == lws_filetype_js) || (source->filetype == lws_filetype_mjs))
        {
            /* Decode */
            {
                int tmpret;
                ec_encoding_t enc;
                ec_encoding_utf8_init(&enc);
                tmpret = ec_encoding_decode(&enc, &uscript, (const ec_byte_t *)source_data, source_len);
                free(source_data); source_data = NULL;
                if (tmpret != 0)
                { fprintf(stderr, "error: invalid source file encoding (only supports UTF-8)\n"); goto fail; }
                if (uscript == NULL)
                { fprintf(stderr, "error: internal error on encoding convertion\n"); goto fail; }
            }

            /* Tokenize */
            tokens = xjs_lexer_start_ex(&err, uscript, \
                    source->source_path);
            ec_delete(uscript);
            if (tokens == NULL) { goto fail; }
            uscript = NULL;

            /* Parse */
            ast = xjs_parser_start_ex(&err, tokens, \
                    (source->filetype == lws_filetype_mjs ? xjs_true : xjs_false), \
                    source->source_path, xjs_true);
            ec_delete(tokens);
            if (ast == NULL) { goto fail; }
            tokens = NULL;

            /* C0 */
            cfg = xjs_c0_start_ex(&err, ast, \
                    source->source_path);
            ec_delete(ast);
            if (cfg == NULL) { goto fail; }
            ast = NULL;

            /* Generate IR */
            new_ir = xjs_c2_start_ex(&err, cfg, \
                    source->source_path);
            ec_delete(cfg);
            if (new_ir == NULL) { goto fail; }
            cfg = NULL;
        }
        else if (source->filetype == lws_filetype_x)
        {
            if (xjs_l0_start(&err, &new_ir, source_data, source_len) != 0)
            {
                free(source_data);
                goto fail;
            }
        }
        else
        { fprintf(stderr, "error: internal error\n"); goto fail; }

        {
            /* Module Full Path & Name */
            {
                char *module_fullpath;
                const char *module_name;
                size_t module_name_len;
                size_t module_fullpath_len;

                module_fullpath = realpath_get(source->source_path);
                if (module_fullpath == NULL)
                { fprintf(stderr, "error: out of memory\n"); goto fail; }
                module_fullpath_len = strlen(module_fullpath);

                if (mainname_get(&module_name, &module_name_len, module_fullpath, strlen(module_fullpath)) != 0)
                { free(module_fullpath); fprintf(stderr, "error: out of memory\n"); goto fail; }
                (void)module_name_len;

                /* TODO: module name duplicate checking */
                xjs_ir_module_fullpath_set(new_ir, module_fullpath, module_fullpath_len);
                xjs_ir_module_name_set(new_ir, module_name, module_name_len);

                free(module_fullpath);
            }

            irs[i] = new_ir; new_ir = NULL;
            i++;
        }
    }

    {
        char stdlib_path[XJSCD_PATH_MAX];
        char *stdlib_source_data = NULL;
        size_t stdlib_source_len = 0;

        /* Load stdlib.js */
        {
            char exe_path[XJSCD_PATH_MAX];
            {
                char *p;
                if (current_program_path(exe_path, XJSCD_PATH_MAX) != 0)
                { fprintf(stderr, "error: get executable path failed\n"); goto fail; }
                p = strrchr(exe_path, OS_SEP);
                if (p == NULL)
                { fprintf(stderr, "error: invalid executable path\n"); goto fail; }
                *p = '\0';
            }
            snprintf(stdlib_path, XJSCD_PATH_MAX, "%s%c%s%c%s", \
                    exe_path, OS_SEP, LWS_LIB_NAME, OS_SEP, LWS_STDLIB_FILENAME);

            if (read_file(&stdlib_source_data, &stdlib_source_len, stdlib_path) != 0)
            {
                fprintf(stderr, "error: failed to open file %s\n", stdlib_path);
                goto fail;
            }
        }

        {
            if (xjs_l1_start(&err, \
                        &ir_merged, \
                        irs, srcs_count, \
                        entry, xjs_false, \
                        stdlib_source_data, stdlib_source_len, \
                        xjs_load_sys_lib_cb) != 0)
            {
                if (stdlib_source_data != NULL) { free(stdlib_source_data); stdlib_source_data = NULL; }
                goto fail;
            }
            if (stdlib_source_data != NULL) { free(stdlib_source_data); stdlib_source_data = NULL; }
            for (i = 0; i != srcs_count; i++)
            {
                if (irs[i] != NULL)
                { ec_delete(irs[i]); }
            }
            free(irs); irs = NULL;
        }

    }

    {
        if (xjs_c4_start_ex(&err, &bytecode, &bytecode_len, ir_merged, xjs_true) != 0)
        { goto fail; }
        ec_delete(ir_merged); ir_merged = NULL;
    }

    *bytecode_out = bytecode;
    *bytecode_len_out = bytecode_len;
    bytecode = NULL;

    goto done;
fail:
    if (err.error_no != 0)
    {
        print_error(&err);
    }
    ret = -1;
done:
    xjs_error_uninit(&err);
    if (irs != NULL)
    {
        for (i = 0; i != srcs_count; i++)
        {
            if (irs[i] != NULL)
            { ec_delete(irs[i]); }
        }
        free(irs);
    }
    if (ir_merged != NULL) ec_delete(ir_merged);
    ec_delete(new_ir);
    if (bytecode != NULL) ec_free(bytecode);
    return ret;
}

