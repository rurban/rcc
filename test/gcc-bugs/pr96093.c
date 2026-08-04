/* GCC Bug #96093 - error recovery after missing a `;` after a function declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96093
 */


#define cdParser

 #include <stdio.h>
 #include <ctype.h>
 #include "error.h"
 #include "lexer.h"
 #include "../lib/mstack.h"
 #include "ast.h"
 sym_t 	cdParserGetToken			(void) ; 					// parser.c

// 	node_t* cdParserTerm				(void) ; 					// expr.c
// 	node_t* cdParserMulDivMod			(void) ; 					// expr.c
// 	node_t* cdParserAddSub				(void) ; 					// expr.c
// 	node_t* cdParserExpr				(void) ; 					// expr.c
// 	node_t* cdParserScan				(char* fileSourceName ); 	// parser.c

// 	node_t* cdParserDeclGlobalConst		(void) ;				    // decl.c
 typedef struct parser_s
 {
  sym_t 	(*getToken)			(void) ;
// 		node_t* (*term)				(void) ;
// 		node_t* (*mulDivMod)		(void) ;
// 		node_t* (*addSub)			(void) ;
// 		node_t* (*expr)				(void) ;
// 		node_t* (*scan)				(char* fileSourceName ) ;
// 		node_t* (*declGlobalConst)	(void);
 } parser_t;
if i omit ';' in parser.h

 [ok]	node_t* cdParserDeclGlobalConst		(void) ;				    // decl.c
// 	[ko]	node_t* cdParserDeclGlobalConst		(void)  				    // decl.c
// 	Copyright (C) 2019 Free Software Foundation, Inc.
// 	  332 | __MATHCALLX (fmin,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  335 | __MATHCALL (fma,, (_Mdouble_ __x, _Mdouble_ __y, _Mdouble_ __z));
// 	  335 | __MATHCALL (fma,, (_Mdouble_ __x, _Mdouble_ __y, _Mdouble_ __z));
// 	  340 | __MATHCALLX (roundeven,, (_Mdouble_ __x), (__const__));
// 	  340 | __MATHCALLX (roundeven,, (_Mdouble_ __x), (__const__));
// 	  344 | __MATHDECL (__intmax_t, fromfp,, (_Mdouble_ __x, int __round,
// 	  344 | __MATHDECL (__intmax_t, fromfp,, (_Mdouble_ __x, int __round,
// 	  349 | __MATHDECL (__uintmax_t, ufromfp,, (_Mdouble_ __x, int __round,
// 	  349 | __MATHDECL (__uintmax_t, ufromfp,, (_Mdouble_ __x, int __round,
// 	  355 | __MATHDECL (__intmax_t, fromfpx,, (_Mdouble_ __x, int __round,
// 	  355 | __MATHDECL (__intmax_t, fromfpx,, (_Mdouble_ __x, int __round,
// 	  361 | __MATHDECL (__uintmax_t, ufromfpx,, (_Mdouble_ __x, int __round,
// 	  361 | __MATHDECL (__uintmax_t, ufromfpx,, (_Mdouble_ __x, int __round,
// 	  365 | __MATHCALLX (fmaxmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  365 | __MATHCALLX (fmaxmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  368 | __MATHCALLX (fminmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  368 | __MATHCALLX (fminmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  371 | __MATHDECL_1 (int, canonicalize,, (_Mdouble_ *__cx, const _Mdouble_ *__x));
// 	  376 | __MATHDECL_1 (int, totalorder,, (const _Mdouble_ *__x,
// 	  381 | __MATHDECL_1 (int, totalordermag,, (const _Mdouble_ *__x,
// 	  386 | __MATHCALL (getpayload,, (const _Mdouble_ *__x));
// 	  386 | __MATHCALL (getpayload,, (const _Mdouble_ *__x));
// 	  389 | __MATHDECL_1 (int, setpayload,, (_Mdouble_ *__x, _Mdouble_ __payload));
// 	  392 | __MATHDECL_1 (int, setpayloadsig,, (_Mdouble_ *__x, _Mdouble_ __payload));
// 	   53 | __MATHCALL (acos,, (_Mdouble_ __x));
// 	   53 | __MATHCALL (acos,, (_Mdouble_ __x));
// 	   55 | __MATHCALL (asin,, (_Mdouble_ __x));
// 	   55 | __MATHCALL (asin,, (_Mdouble_ __x));
// 	   57 | __MATHCALL (atan,, (_Mdouble_ __x));
// 	   57 | __MATHCALL (atan,, (_Mdouble_ __x));
// 	   59 | __MATHCALL (atan2,, (_Mdouble_ __y, _Mdouble_ __x));
// 	   59 | __MATHCALL (atan2,, (_Mdouble_ __y, _Mdouble_ __x));
// 	   62 | __MATHCALL_VEC (cos,, (_Mdouble_ __x));
// 	   62 | __MATHCALL_VEC (cos,, (_Mdouble_ __x));
// 	   64 | __MATHCALL_VEC (sin,, (_Mdouble_ __x));
// 	   64 | __MATHCALL_VEC (sin,, (_Mdouble_ __x));
// 	   66 | __MATHCALL (tan,, (_Mdouble_ __x));
// 	   66 | __MATHCALL (tan,, (_Mdouble_ __x));
// 	   71 | __MATHCALL (cosh,, (_Mdouble_ __x));
// 	   71 | __MATHCALL (cosh,, (_Mdouble_ __x));
// 	   73 | __MATHCALL (sinh,, (_Mdouble_ __x));
// 	   73 | __MATHCALL (sinh,, (_Mdouble_ __x));
// 	   75 | __MATHCALL (tanh,, (_Mdouble_ __x));
// 	   75 | __MATHCALL (tanh,, (_Mdouble_ __x));
// 	   79 | __MATHDECL_VEC (void,sincos,,
// 	   79 | __MATHDECL_VEC (void,sincos,,
// 	   85 | __MATHCALL (acosh,, (_Mdouble_ __x));
// 	   85 | __MATHCALL (acosh,, (_Mdouble_ __x));
// 	   87 | __MATHCALL (asinh,, (_Mdouble_ __x));
// 	   87 | __MATHCALL (asinh,, (_Mdouble_ __x));
// 	   89 | __MATHCALL (atanh,, (_Mdouble_ __x));
// 	   89 | __MATHCALL (atanh,, (_Mdouble_ __x));
// 	   95 | __MATHCALL_VEC (exp,, (_Mdouble_ __x));
// 	   95 | __MATHCALL_VEC (exp,, (_Mdouble_ __x));
// 	   98 | __MATHCALL (frexp,, (_Mdouble_ __x, int *__exponent));
// 	   98 | __MATHCALL (frexp,, (_Mdouble_ __x, int *__exponent));
// 	  101 | __MATHCALL (ldexp,, (_Mdouble_ __x, int __exponent));
// 	  101 | __MATHCALL (ldexp,, (_Mdouble_ __x, int __exponent));
// 	  104 | __MATHCALL_VEC (log,, (_Mdouble_ __x));
// 	  104 | __MATHCALL_VEC (log,, (_Mdouble_ __x));
// 	  107 | __MATHCALL (log10,, (_Mdouble_ __x));
// 	  107 | __MATHCALL (log10,, (_Mdouble_ __x));
// 	  110 | __MATHCALL (modf,, (_Mdouble_ __x, _Mdouble_ *__iptr)) __nonnull ((2));
// 	  110 | __MATHCALL (modf,, (_Mdouble_ __x, _Mdouble_ *__iptr)) __nonnull ((2));
// 	  114 | __MATHCALL (exp10,, (_Mdouble_ __x));
// 	  114 | __MATHCALL (exp10,, (_Mdouble_ __x));
// 	  119 | __MATHCALL (expm1,, (_Mdouble_ __x));
// 	  119 | __MATHCALL (expm1,, (_Mdouble_ __x));
// 	  122 | __MATHCALL (log1p,, (_Mdouble_ __x));
// 	  122 | __MATHCALL (log1p,, (_Mdouble_ __x));
// 	  125 | __MATHCALL (logb,, (_Mdouble_ __x));
// 	  125 | __MATHCALL (logb,, (_Mdouble_ __x));
// 	  130 | __MATHCALL (exp2,, (_Mdouble_ __x));
// 	  130 | __MATHCALL (exp2,, (_Mdouble_ __x));
// 	  133 | __MATHCALL (log2,, (_Mdouble_ __x));
// 	  133 | __MATHCALL (log2,, (_Mdouble_ __x));
// 	  140 | __MATHCALL_VEC (pow,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  140 | __MATHCALL_VEC (pow,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  143 | __MATHCALL (sqrt,, (_Mdouble_ __x));
// 	  143 | __MATHCALL (sqrt,, (_Mdouble_ __x));
// 	  147 | __MATHCALL (hypot,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  147 | __MATHCALL (hypot,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  152 | __MATHCALL (cbrt,, (_Mdouble_ __x));
// 	  152 | __MATHCALL (cbrt,, (_Mdouble_ __x));
// 	  159 | __MATHCALLX (ceil,, (_Mdouble_ __x), (__const__));
// 	  159 | __MATHCALLX (ceil,, (_Mdouble_ __x), (__const__));
// 	  162 | __MATHCALLX (fabs,, (_Mdouble_ __x), (__const__));
// 	  162 | __MATHCALLX (fabs,, (_Mdouble_ __x), (__const__));
// 	  165 | __MATHCALLX (floor,, (_Mdouble_ __x), (__const__));
// 	  165 | __MATHCALLX (floor,, (_Mdouble_ __x), (__const__));
// 	  168 | __MATHCALL (fmod,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  168 | __MATHCALL (fmod,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  196 | __MATHCALLX (copysign,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  196 | __MATHCALLX (copysign,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  201 | __MATHCALL (nan,, (const char *__tagb));
// 	  201 | __MATHCALL (nan,, (const char *__tagb));
// 	  217 | __MATHCALL (j0,, (_Mdouble_));
// 	  217 | __MATHCALL (j0,, (_Mdouble_));
// 	  218 | __MATHCALL (j1,, (_Mdouble_));
// 	  218 | __MATHCALL (j1,, (_Mdouble_));
// 	  219 | __MATHCALL (jn,, (int, _Mdouble_));
// 	  219 | __MATHCALL (jn,, (int, _Mdouble_));
// 	  220 | __MATHCALL (y0,, (_Mdouble_));
// 	  220 | __MATHCALL (y0,, (_Mdouble_));
// 	  221 | __MATHCALL (y1,, (_Mdouble_));
// 	  221 | __MATHCALL (y1,, (_Mdouble_));
// 	  222 | __MATHCALL (yn,, (int, _Mdouble_));
// 	  222 | __MATHCALL (yn,, (int, _Mdouble_));
// 	  228 | __MATHCALL (erf,, (_Mdouble_));
// 	  228 | __MATHCALL (erf,, (_Mdouble_));
// 	  229 | __MATHCALL (erfc,, (_Mdouble_));
// 	  229 | __MATHCALL (erfc,, (_Mdouble_));
// 	  230 | __MATHCALL (lgamma,, (_Mdouble_));
// 	  230 | __MATHCALL (lgamma,, (_Mdouble_));
// 	  235 | __MATHCALL (tgamma,, (_Mdouble_));
// 	  235 | __MATHCALL (tgamma,, (_Mdouble_));
// 	  249 | __MATHCALL (lgamma,_r, (_Mdouble_, int *__signgamp));
// 	  249 | __MATHCALL (lgamma,_r, (_Mdouble_, int *__signgamp));
// 	  256 | __MATHCALL (rint,, (_Mdouble_ __x));
// 	  256 | __MATHCALL (rint,, (_Mdouble_ __x));
// 	  259 | __MATHCALL (nextafter,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  259 | __MATHCALL (nextafter,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  266 | __MATHCALL (nextdown,, (_Mdouble_ __x));
// 	  266 | __MATHCALL (nextdown,, (_Mdouble_ __x));
// 	  268 | __MATHCALL (nextup,, (_Mdouble_ __x));
// 	  268 | __MATHCALL (nextup,, (_Mdouble_ __x));
// 	  272 | __MATHCALL (remainder,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  272 | __MATHCALL (remainder,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  276 | __MATHCALL (scalbn,, (_Mdouble_ __x, int __n));
// 	  276 | __MATHCALL (scalbn,, (_Mdouble_ __x, int __n));
// 	  280 | __MATHDECL (int,ilogb,, (_Mdouble_ __x));
// 	  280 | __MATHDECL (int,ilogb,, (_Mdouble_ __x));
// 	  285 | __MATHDECL (long int, llogb,, (_Mdouble_ __x));
// 	  285 | __MATHDECL (long int, llogb,, (_Mdouble_ __x));
// 	  290 | __MATHCALL (scalbln,, (_Mdouble_ __x, long int __n));
// 	  290 | __MATHCALL (scalbln,, (_Mdouble_ __x, long int __n));
// 	  294 | __MATHCALL (nearbyint,, (_Mdouble_ __x));
// 	  294 | __MATHCALL (nearbyint,, (_Mdouble_ __x));
// 	  298 | __MATHCALLX (round,, (_Mdouble_ __x), (__const__));
// 	  298 | __MATHCALLX (round,, (_Mdouble_ __x), (__const__));
// 	  302 | __MATHCALLX (trunc,, (_Mdouble_ __x), (__const__));
// 	  302 | __MATHCALLX (trunc,, (_Mdouble_ __x), (__const__));
// 	  307 | __MATHCALL (remquo,, (_Mdouble_ __x, _Mdouble_ __y, int *__quo));
// 	  307 | __MATHCALL (remquo,, (_Mdouble_ __x, _Mdouble_ __y, int *__quo));
// 	  314 | __MATHDECL (long int,lrint,, (_Mdouble_ __x));
// 	  314 | __MATHDECL (long int,lrint,, (_Mdouble_ __x));
// 	  316 | __MATHDECL (long long int,llrint,, (_Mdouble_ __x));
// 	  320 | __MATHDECL (long int,lround,, (_Mdouble_ __x));
// 	  320 | __MATHDECL (long int,lround,, (_Mdouble_ __x));
// 	  322 | __MATHDECL (long long int,llround,, (_Mdouble_ __x));
// 	  326 | __MATHCALL (fdim,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  326 | __MATHCALL (fdim,, (_Mdouble_ __x, _Mdouble_ __y));
// 	  329 | __MATHCALLX (fmax,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  329 | __MATHCALLX (fmax,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  332 | __MATHCALLX (fmin,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  332 | __MATHCALLX (fmin,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  335 | __MATHCALL (fma,, (_Mdouble_ __x, _Mdouble_ __y, _Mdouble_ __z));
// 	  335 | __MATHCALL (fma,, (_Mdouble_ __x, _Mdouble_ __y, _Mdouble_ __z));
// 	  340 | __MATHCALLX (roundeven,, (_Mdouble_ __x), (__const__));
// 	  340 | __MATHCALLX (roundeven,, (_Mdouble_ __x), (__const__));
// 	  344 | __MATHDECL (__intmax_t, fromfp,, (_Mdouble_ __x, int __round,
// 	  344 | __MATHDECL (__intmax_t, fromfp,, (_Mdouble_ __x, int __round,
// 	  349 | __MATHDECL (__uintmax_t, ufromfp,, (_Mdouble_ __x, int __round,
// 	  349 | __MATHDECL (__uintmax_t, ufromfp,, (_Mdouble_ __x, int __round,
// 	  355 | __MATHDECL (__intmax_t, fromfpx,, (_Mdouble_ __x, int __round,
// 	  355 | __MATHDECL (__intmax_t, fromfpx,, (_Mdouble_ __x, int __round,
// 	  361 | __MATHDECL (__uintmax_t, ufromfpx,, (_Mdouble_ __x, int __round,
// 	  361 | __MATHDECL (__uintmax_t, ufromfpx,, (_Mdouble_ __x, int __round,
// 	  365 | __MATHCALLX (fmaxmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  365 | __MATHCALLX (fmaxmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  368 | __MATHCALLX (fminmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  368 | __MATHCALLX (fminmag,, (_Mdouble_ __x, _Mdouble_ __y), (__const__));
// 	  371 | __MATHDECL_1 (int, canonicalize,, (_Mdouble_ *__cx, const _Mdouble_ *__x));
// 	  376 | __MATHDECL_1 (int, totalorder,, (const _Mdouble_ *__x,
// 	  381 | __MATHDECL_1 (int, totalordermag,, (const _Mdouble_ *__x,
// 	  386 | __MATHCALL (getpayload,, (const _Mdouble_ *__x));
// 	  386 | __MATHCALL (getpayload,, (const _Mdouble_ *__x));
// 	  389 | __MATHDECL_1 (int, setpayload,, (_Mdouble_ *__x, _Mdouble_ __payload));
// 	  392 | __MATHDECL_1 (int, setpayloadsig,, (_Mdouble_ *__x, _Mdouble_ __payload));
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	   24 | __MATHCALL_NARROW (__MATHCALL_NAME (add), __MATHCALL_REDIR_NAME (add), 2);
// 	   27 | __MATHCALL_NARROW (__MATHCALL_NAME (div), __MATHCALL_REDIR_NAME (div), 2);
// 	   30 | __MATHCALL_NARROW (__MATHCALL_NAME (mul), __MATHCALL_REDIR_NAME (mul), 2);
// 	   33 | __MATHCALL_NARROW (__MATHCALL_NAME (sub), __MATHCALL_REDIR_NAME (sub), 2);
// 	  773 | extern int signgam;
// 	   23 | extern int __iscanonicall (long double __x)


