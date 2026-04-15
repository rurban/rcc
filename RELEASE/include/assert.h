#ifndef RCC_ASSERT_H
#define RCC_ASSERT_H

#include <stdlib.h>

#define assert(expr) ((expr) ? (void)0 : abort())

#endif
