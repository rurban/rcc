/* GCC Bug #117810 - Feature request: attribute access but for (start, end) type interfaces
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117810
 */
/* { dg-do compile } */

#include <stddef.h>

/* Feature request: __attribute__((access)) only supports (ptr, len)
 * interfaces, not (start, end-pointer) iterator-style interfaces.  The
 * reporter's real-world function from his LDAP ASN.1 scanner: */
size_t scan_asn1BOOLEAN(const char* src, const char* max, int* l);

/* The loop rationale (comment 2): */
// for (i=0; i<n; ++i) { ... a[i] ... }
// n is not the index of the last element in the array but the one after,
// as counting starts with 0.  max points one past the last element.