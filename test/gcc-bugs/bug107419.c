/* GCC Bug #107419 - attributes are ignored when selecting TLS model
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107419
 */
/* { dg-do compile } */


__attribute__((common))
__thread int i;

int *f()
{
        return &i;
}
// C frontend invokes decl_default_tls_model before processing attributes, assigning local-exec model as if the 'common' attribute was not present. Recomputing it later would select initial-exec model, breaking internal verification that was weakened to solve <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - frontends sometimes select wrong (too strong) TLS access model"
//    href="show_bug.cgi?id=107353">PR 107353</a>.


