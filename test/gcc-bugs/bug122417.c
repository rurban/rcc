/* GCC Bug #122417 - auto using as a storage class but without an identifier diagnostic could be improved; looks like a shadowing declaration that uses auto as a type inference
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122417
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu23" } */

/* C23 auto type inference colliding with a typedef name produces
 * unhelpful diagnostics (the reporter's two programs merged). */
typedef int A;
typedef int B;
int main(){
    auto A=0; /* { dg-error "expected identifier or .(. before .=. token" } */
    auto(B)=0; /* { dg-error "expected identifier or .(. before .B." } */
}
