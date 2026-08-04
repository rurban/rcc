/* GCC Bug #125418 - [16/17 regression] #pragma weak isn't applied hidden TLS symbol
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125418
 */
/* { dg-do compile } */
/* { dg-require-effective-target tls } */
/* { dg-options "-O2 -fno-pie -fdump-ipa-whole-program" } */

/* tls_model("initial-exec") must not be upgraded to local-exec even when
   the symbol is hidden and -fno-pie is used. */
__attribute__((visibility("hidden")))
__attribute__((tls_model("initial-exec")))
extern __thread int x;

void reference(void) { x++; }

/* { dg-final { scan-ipa-dump "Varpool flags: tls-initial-exec" "whole-program" } } */

// With GCC 16 (regression): the whole-program IPA dump shows
// "tls-local-exec" instead of "tls-initial-exec".  GCC 15, or this bug's
// proposed fix applied, keeps "tls-initial-exec" as declared.
//
// Root cause (comment 0): commit 8cad8f94b45 (PR c/107419) made
// c_decl_attributes() unconditionally upgrade a TLS variable's model to
// decl_default_tls_model()'s result whenever that is stronger, even when
// the user explicitly requested a weaker model via the tls_model()
// attribute; the IPA visibility pass already guards the equivalent upgrade
// with !lookup_attribute ("tls_model", DECL_ATTRIBUTES (decl)), but
// c_decl_attributes() lacks that guard.  Proposed fix (gcc/c/c-decl.cc):
//
// --- a/gcc/c/c-decl.cc
// +++ b/gcc/c/c-decl.cc
// @@ -5739,8 +5739,9 @@ c_decl_attributes (tree *node, tree attributes, int flags)
//    if (last_decl == error_mark_node)
//      last_decl = NULL_TREE;
//    tree attr = decl_attributes (node, attributes, flags, last_decl);
// -  if (VAR_P (*node) && DECL_THREAD_LOCAL_P (*node))
// +  if (VAR_P (*node) && DECL_THREAD_LOCAL_P (*node)
// +      && !lookup_attribute ("tls_model", DECL_ATTRIBUTES (*node)))
//      {
// -      // tls_model attribute can set a stronger TLS access model.
//        tls_model model = DECL_TLS_MODEL (*node);
//        tls_model default_model = decl_default_tls_model (*node);
//        if (default_model > model)
// (patch continues unchanged below this hunk)


