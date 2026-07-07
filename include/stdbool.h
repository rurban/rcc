#ifndef RCC_STDBOOL_H
#define RCC_STDBOOL_H

#define __bool_true_false_are_defined 1

/* In C23, bool/true/false are keywords; <stdbool.h> must not redefine them as
 * macros (doing so would give true/false type int instead of bool).  Only the
 * pre-C23 compatibility macros are provided in older modes. */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
#define bool _Bool
#define true 1
#define false 0
#endif

#endif
