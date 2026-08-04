/* GCC Bug #122417 - auto using as a storage class but without an identifier diagnostic could be improved; looks like a shadowing declaration that uses auto as a type inference
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122417
 */


typedef int A;
int main(){
    auto A=0;
}
// Error 1:
// <source>:3:11: error: expected identifier or '(' before '=' token
//     3 |     auto A=0;
//       |           ^
// Program 2:
typedef int B;
int main(){
//     auto(B)=0;
}
// Error 2:
// <source>:3:10: error: expected identifier or '(' before 'B'
//     3 |     auto(B)=0;
//       |          ^
// The message in error 1 is correct, but it would be helpful to point out specifically here that this cannot declare A. Also a pointer operator * or attribute specifier sequence [[...]] could validly occur after A (e.g. auto A[[]]x=0;), so it's not even comprehensive either. The message in error 2 is outright confusing, B is an identifier and the token before B is an opening parenthesis. I think it is worth improving the error messages in both programs to specifically mention that a typedef being visible with the same name prevents type inference from being used.


