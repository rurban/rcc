/* GCC Bug #71157 - -Wnull-dereference false alarm in wrong function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71157
 */


_402 ={v} [<a class="bz_bug_link 
  # USE = nonlocal null { D.5991 } (nonlocal, escaped)
  # CLB = nonlocal null { D.5991 } (nonlocal, escaped)
    goto <bb 61>;
//   else
    goto <bb 109>;
// ;;    succ:       61 [85.0%]  (TRUE_VALUE,EXECUTABLE)
// ;;                109 [15.0%]  (FALSE_VALUE,EXECUTABLE)
// The location shown is just an artifact of merging expressions and not preserving the right locations. The middle-end is not very smart at that, this is why middle-end warnings are often confusing.
// The testcase is too large for me to analyze further.


