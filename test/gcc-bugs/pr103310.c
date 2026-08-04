/* GCC Bug #103310 - null comparison with a weak symbol eliminated
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103310
 */


extern void alias (void);

void call_alias (void)
{
  __builtin_printf ("in %s: alias = %p\n", __func__, alias);

  if (alias)
    alias ();
}

void call_ptr_alias (void)
{
  void (*p)(void) = alias;

  __builtin_printf ("in %s: alias = %p\n", __func__, p);

  if (p)
    p ();
}

extern void alias (void)  __attribute__((weak));
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - null comparison with a weak symbol eliminated"
//    href="show_bug.cgi?id=103310">pr103310</a>.c: In function ‘call_alias’:
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - null comparison with a weak symbol eliminated"
//    href="show_bug.cgi?id=103310">pr103310</a>.c:7:7: warning: the address of ‘alias’ will always evaluate as ‘true’ [-Waddress]
//     7 |   if (alias)
//       |       ^~~~~
// ;; Function call_alias (null)
// ;; enabled by -tree-original
{
  static const char __func__[11] = "call_alias";

    static const char __func__[11] = "call_alias";
  __builtin_printf ((const char *) "in %s: alias = %p\n", (const char *) &__func__, alias);
  if (1)
    {
      alias ();
    }
}
// ;; Function call_ptr_alias (null)
// ;; enabled by -tree-original
{
  void (*<T349>) (void) p = alias;
  static const char __func__[15] = "call_ptr_alias";

    static const char __func__[15] = "call_ptr_alias";
    void (*<T349>) (void) p = alias;
  __builtin_printf ((const char *) "in %s: alias = %p\n", (const char *) &__func__, p);
  if (p != 0B)
    {
      p ();
    }
}
// ;; Function call_alias (call_alias, funcdef_no=0, decl_uid=1945, cgraph_uid=1, symbol_order=0)

void call_alias ()
{
  static const char __func__[11] = "call_alias";

//   <bb 2> [local count: 1073741824]:
  __builtin_printf ("in %s: alias = %p\n", &__func__, alias);
  alias (); [tail call]
//   return;

}
// ;; Function call_ptr_alias (call_ptr_alias, funcdef_no=1, decl_uid=1949, cgraph_uid=2, symbol_order=1)
// Removing basic block 5
void call_ptr_alias ()
{
  static const char __func__[15] = "call_ptr_alias";

//   <bb 2> [local count: 1073741824]:
  __builtin_printf ("in %s: alias = %p\n", &__func__, alias);
  if (alias != 0B)
    goto <bb 3>; [53.47%]
//   else
    goto <bb 4>; [46.53%]

//   <bb 3> [local count: 574129753]:
  alias (); [tail call]

//   <bb 4> [local count: 1073741824]:
//   return;

}


