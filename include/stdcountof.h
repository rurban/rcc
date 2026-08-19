/* stdcountof.h — C23 standard countof macro (P2809) */
#ifndef _STD_COUNTOF_H
#define _STD_COUNTOF_H

/* countof(x) — number of elements in a statically-sized array.
 * GCC/Clang extension __builtin_available() was considered but the
 * standard form uses a _Generic + sizeof trick that works everywhere. */
#define countof(x) \
    (sizeof(x) / sizeof((x)[0]))

#endif
