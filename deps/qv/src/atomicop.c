/* Atomic
 * Copyright(c) 2016-2018 y2c2 */

#include "qv_config.h"

#ifdef _MSC_VER
/* InterlockedCompareExchange */
#include <Windows.h>
#endif

#include "atomicop.h"

/****************************************************************
 * Reference:
 *
 * <<Pre-defined Compiler Macros>>:
 * https://sourceforge.net/p/predef/wiki/Architectures/
 *
 * <<Built-in functions for atomic memory access>>
 * https://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html
 *
 * <<Synchronization Functions>> -- Interlocked Functions
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx#interlocked_functions
 * 
 * <<Simple, Fast, and Practical Non-Blocking and Blocking
 * Concurrent Queue Algorithms>>
 * https://www.research.ibm.com/people/m/michael/podc-1996.pdf
 *
 ****************************************************************/


int qv_cas(int *ptr, int oldval, int newval)
{
#if defined(__GNUC__)

/* Compiler: GCC */
#if defined(__i386__) || defined(__amd64__)
    int out;
    __asm__ __volatile__ (
            "lock; cmpxchg %2, %1;"
            : "=a" (out), "+m" (*(volatile int*) ptr)
            : "r" (newval), "0" (oldval)
            : "memory");
    return out;
#else
    return __sync_val_compare_and_swap(ptr, oldval, newval);
#endif

#elif defined(_MSC_VER)

    /* Compiler: MSVC */
    return InterlockedCompareExchange( \
            ptr, oldval, newval);

#else

/* Compiler: Undefined */
#error "unknown compiler"

#endif
}


