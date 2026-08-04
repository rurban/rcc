/* GCC Bug #78989 - Missing -Waddress warning due to -Wno-system-headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78989
 */


{
 return (asan_poison_variables &&  
# 6 "gimplify.cpp" 3 4
//                               __null
//                                   );
}
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Missing -Waddress warning due to -Wno-system-headers"
//    href="show_bug.cgi?id=78989">pr78989</a>.ii: In function ‘int asan_poison_variables()’:
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Missing -Waddress warning due to -Wno-system-headers"
//    href="show_bug.cgi?id=78989">pr78989</a>.ii:4:10: warning: the address of ‘int asan_poison_variables()’ will never be NULL [-Waddress]
//     4 |  return (asan_poison_variables &&
//       |          ^~~~~~~~~~~~~~~~~~~~~
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Missing -Waddress warning due to -Wno-system-headers"
//    href="show_bug.cgi?id=78989">pr78989</a>.ii:2:1: note: ‘int asan_poison_variables()’ declared here
//     2 | asan_poison_variables ()
//       | ^~~~~~~~~~~~~~~~~~~~~


