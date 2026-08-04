/* GCC Bug #81756 - type attributes silently ignored on type declarations
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81756
 */
/* { dg-do compile } */


struct __attribute__((aligned(64))) foo;
struct foo { char i; };

// _Static_assert(__alignof__(struct foo) == 64, "decl attribute ignored!");
// ```
// The attribute of the declaration in line 1 should be inherited by the definition in line 2, making the assert in line 4 pass. However, basically all versions of gcc fail the assert, as can be seen on godbolt: <a href="https://godbolt.org/z/ssW7T363W">https://godbolt.org/z/ssW7T363W</a>
// Clang seems to comply to gcc's documentation much better, starting as early as of clang 3.2. Again, here's the godbolt link: <a href="https://godbolt.org/z/MnvzsvcGG">https://godbolt.org/z/MnvzsvcGG</a>
// Newer versions of gcc supporting the C23 attribute syntax do pass the test when specifying attributes using the standard syntax, as can be seen here: <a href="https://godbolt.org/z/8Ee4E3exP">https://godbolt.org/z/8Ee4E3exP</a>
// However, I don't see a reason why it should be limited to only the C23 attribute syntax. Especially, as the GNU syntax gets *silently* ignored and clang already unconditionally excepts both variants.
// The following change fixes -- at least from my understanding -- the broken behaviour in gcc:
// diff --git a/gcc/c/c-parser.cc b/gcc/c/c-parser.cc
// index 00f8bf4376e5..02cf46f199d7 100644
// --- a/gcc/c/c-parser.cc
// +++ b/gcc/c/c-parser.cc
// @@ -4114,7 +4114,8 @@ c_parser_struct_or_union_specifier (c_parser *parser)
     c_parser_error (parser, "expected %<;%>");
//    /* ??? Existing practice is that GNU attributes are ignored after
//       the struct or union keyword when not defining the members.  */
// -  ret = parser_xref_tag (ident_loc, code, ident, have_std_attrs, std_attrs,
// +  ret = parser_xref_tag (ident_loc, code, ident, have_std_attrs || attrs,
// +                        std_attrs ? chainon (std_attrs, attrs) : attrs,
//                          false);
   return ret;
 }

// It's far from a proper change, as there are more places that need to be adapted to, for example, cover enums as well. However, it shows where things go wrong and where the (still parsed!) (GNU only!) attributes get silently ignored. And yes, the comment needs a change too.
// If there's consent that gcc should follow Clang's lead and actually comply to its own documentation, I can prepare a proper patch.
// Thanks,
// Mathias


