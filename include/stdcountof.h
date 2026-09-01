/* stdcountof.h -- C2Y <stdcountof.h> (WG14 N3369/N3469): countof(x)
 * expands to the _Countof operator, which yields the number of elements
 * of an array operand (a hard error on anything else, unlike the old
 * sizeof(x)/sizeof(x[0]) trick, which silently miscomputes for a decayed
 * pointer). */
#ifndef _STD_COUNTOF_H
#define _STD_COUNTOF_H

#define countof(x) _Countof(x)

#endif
