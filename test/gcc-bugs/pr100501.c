/* GCC Bug #100501 - ICE with inline-asm and void statement expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100501
 */
/* { dg-do compile } */

void foo() {
  __asm__(""
          :
          : "m"(({ /* { dg-error "lvalue required as left operand of assignment" } */
            if (8)
              ;
          })));
}

