#include <stdlib.h>
#include <stdio.h>
#include "qv.h"
#include "test_loop.h"

int main(void)
{
    qv_allocator_set_malloc(malloc);
    qv_allocator_set_free(free);

    test_loop();

    return 0;
}
