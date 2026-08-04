/* GCC Bug #116092 - Should allow --param options to the optimize attribute/pragma
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116092
 */


void __attribute__((optimize("--param=max-completely-peel-loop-nest-depth=1")))
// bar (void)
{}
// ```
// Currently the front-ends reject this with:
// <source>:4:1: warning: bad option '--param=max-completely-peel-loop-nest-depth=1' to attribute 'optimize' [-Wattributes]
//     4 | {}
//       | ^
// This is due to code in the c-family/c-common.cc:
// ```
//               /* If the user supplied -Oxxx or -fxxx, only allow -Oxxx or -fxxx
//                  options.  */
              if (*p == '-' && p[1] != 'O' && p[1] != 'f')
                {
// ```
// We should allow param options to change too I think and not just -f* -O* options.


