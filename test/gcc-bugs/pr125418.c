/* GCC Bug #125418 - [16/17 regression] #pragma weak isn't applied hidden TLS symbol
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125418
 */


// /* tls_model("initial-exec") must not be upgraded to local-exec even when
//    the symbol is hidden and -fno-pie is used. */
__attribute__((visibility("hidden")))
__attribute__((tls_model("initial-exec")))
extern __thread int x;

void reference(void) { x++; }

// ```
// With GCC 16 (regression): dump shows tls-local-exec. With GCC 15 or fix applied: tls-initial-exec. Verified with clean builds of all three states.
Fix:
// ```
// --- a/gcc/c/c-decl.cc
// +++ b/gcc/c/c-decl.cc
// @@ -5739,8 +5739,9 @@ c_decl_attributes (tree *node, tree attributes, int flags)
   if (last_decl == error_mark_node)
     last_decl = NULL_TREE;
   tree attr = decl_attributes (node, attributes, flags, last_decl);
// -  if (VAR_P (*node) && DECL_THREAD_LOCAL_P (*node))
// +  if (VAR_P (*node) && DECL_THREAD_LOCAL_P (*node)
// +      && !lookup_attribute ("tls_model", DECL_ATTRIBUTES (*node)))
     {
// -      // tls_model attribute can set a stronger TLS access model.
       tls_model model = DECL_TLS_MODEL (*node);
       tls_model default_model = decl_default_tls_model (*node);
       if (default_model > model)
// ```


