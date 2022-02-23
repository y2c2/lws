/* Lightweight Service : Library : Standard Libraries
 * Copyright(c) 2017-2018 y2c2 */

#include "lws_lib_os.h"
#include "lws_lib_tcpwrap.h"
#include "lws_lib_httpwrap.h"

#include "lws_stdlib.h"

int lws_stdlib_load(xjr_vm *vm)
{
    if (xjr_vm_lib_install(vm, "os", lws_lib_os) != 0) { goto fail; }
    if (xjr_vm_lib_install(vm, "tcpwrap", lws_lib_tcpwrap) != 0) { goto fail; }
    if (xjr_vm_lib_install(vm, "httpwrap", lws_lib_httpwrap) != 0) { goto fail; }

    return 0;
fail:
    return -1;
}

