/* GCC Bug #83011 - -Wformat-truncation=2 difficult to  avoid for non-constant bounds
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83011
 */


{
  __SIZE_TYPE__ n = __builtin_strlen (s) + 2;

  char *d = __builtin_malloc (n);

  __builtin_snprintf (d, n, "%s ", s);

  return d;
}

// ;; Function f (f, funcdef_no=0, decl_uid=1891, cgraph_uid=0, symbol_order=0)
// d.c:7: __builtin_snprintf: objsize = 2, fmtstr = "%s "
//   Directive 1 at offset 0: "%s"
//     Result: 0, 1, -1, 9223372036854775807 (0, 1, -1, -1)
//   Directive 2 at offset 2: " ", length = 1
//     Result: 1, 1, 1, 1 (1, 2, -1, -1)
//   Directive 3 at offset 3: "", length = 1
// d.c: In function ‘f’:
// d.c:7:29: warning: ‘__builtin_snprintf’ output may be truncated before the last format character [-Wformat-truncation=]
   __builtin_snprintf (d, n, "%s ", s);
//                              ^~~~~
// d.c:7:3: note: ‘__builtin_snprintf’ output 2 or more bytes (assuming 3) into a destination of size 2
   __builtin_snprintf (d, n, "%s ", s);
//    ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The line "d.c:7: __builtin_snprintf: objsize = 2, fmtstr = "%s "" shows that for the small test case, the checker assumes the size of the destination (objsize) is 2.  In your case it assumes it's just 1.  This is actually a feature (although admittedly not a terribly intuitive one).  At level 2, the warning is designed to be more strict than at level 1, and avoiding it involves essentially proving to the checker that truncation cannot happen.  Under simple circumstances when the size of the destination buffer is constant that's usually fairly straightforward, but in more involved ones like in the test case where the size of the buffer is a function of a number of variables, including the length of a string argument to a %s directive, it can be less obvious.  The checker actually determines that the size is in some range between 1 and some large number.  At level 1, it takes the upper bound of the range to be the size, but at level 2, it takes the lower bound, but the output of "%s " needs at least two bytes.  So to avoid the warning, you need to tell the checker the buffer is bigger than 2 bytes (in your case, preferably bigger than 28 bytes, because any less would imply the len computation wrapped around zero).  The simplest way to do it is by adding an assertion, e.g., like so:

   len = 52 * timer_count + 27 + (prefix == NULL ? 0 : strlen(prefix)) + 1;

    if (len < 28)
//       __builtin_unreachable ();   // or abort()

   buf = xmalloc(len);
// If you compile your test case with this change the warning disappears and you will see the following line in the output of the -fdump-tree-printf-return-value option:
// timer.c:395: snprintf: objsize = 28, fmtstr = "%s "
// indicating the checker uses 28 and the minimum size of the buffer.
// Let me know if this doesn't resolve the problem for you.


