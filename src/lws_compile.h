#ifndef LWS_COMPILE_H
#define LWS_COMPILE_H

#include <stdio.h>
#include "sourcelist.h"

int lws_compile( \
        char **bytecode_out, size_t *bytecode_len_out, \
        const sourcelist_ref sources, \
        const char *entry);

#endif

