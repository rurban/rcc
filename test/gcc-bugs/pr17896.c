/* GCC Bug #17896 - The expression (a>0 & b>0) should give clearer warning message (-Wparentheses)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=17896
 */


foo (a == b & (b == c));   /* { dg-warning "boolean" "correct warning" } */
    foo (a == b & (b == c));   /* { dg-warning "boolean" "correct warning" } */
    foo (a == b & (b == c));   /* { dg-warning "boolean" "correct warning" } */
    foo ((a == b) & b == c);   /* { dg-warning "boolean" "correct warning" } */
    foo ((a == b) & b == c);   /* { dg-warning "boolean" "correct warning" } */
    foo ((a == b) & b == c);   /* { dg-warning "boolean" "correct warning" } */
   foo (a == b | (b == c));   /* { dg-warning "boolean" "correct warning" } */
   foo (a == b | (b == c));   /* { dg-warning "boolean" "correct warning" } */
   foo (a == b | (b == c));   /* { dg-warning "boolean" "correct warning" } */
   foo ((a == b) | b == c);   /* { dg-warning "boolean" "correct warning" } */
   foo ((a == b) | b == c);   /* { dg-warning "boolean" "correct warning" } */
   foo ((a == b) | b == c);   /* { dg-warning "boolean" "correct warning" } */
     foo (a == b & b == c);     /* { dg-warning "boolean" "correct warning" } */
     foo (a == b & (b == c));   /* { dg-warning "boolean" "correct warning" } */
     foo ((a == b) & b == c);   /* { dg-warning "boolean" "correct warning" } */
     foo (++a == b & b == c);   /* { dg-warning "comparison" "correct warning" } */
     foo (a == b | b == c);     /* { dg-warning "boolean" "correct warning" } */
    foo (a == b | (b == c));   /* { dg-warning "boolean" "correct warning" } */
    foo ((a == b) | b == c);   /* { dg-warning "boolean" "correct warning" } */
    foo (++a == b | b == c);   /* { dg-warning "comparison" "correct warning" } */


