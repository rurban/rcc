/* GCC Bug #99826 - GIMPLE FE fails to grok pointer declarators
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99826
 */


char * __GIMPLE(ssa) foo(char *p)
{
  char *_2;

// __BB(2):
  _2 = p_1(D);
  return _2;
}
      unsigned version, ver_offset;
      if (declarator->kind == cdk_id
//           && is_gimple_reg_type (specs->type)
$5 = {kind = cdk_pointer, id_loc = 0, declarator = 0x3775200, u = {id = {
      id = <tree 0x0>, attrs = <tree 0x0>}, arg_info = 0x0, array = {
      vla_unspec_p = 0}, pointer_quals = 0, attrs = <tree 0x0>}}

// (and build the actual pointer type in the SSA case).  The "early-out"
// (and registering it in the binding for future lookup).


