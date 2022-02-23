#include "qv.h"
#include "test_loop.h"

static int test_loop_empty(void)
{
    qv_loop_t loop;
    qv_loop_init(&loop);
    qv_loop_close(&loop);
    return 0;
}

int test_loop(void)
{
    test_loop_empty();
    return 0;
}

