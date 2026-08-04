/* GCC Bug #16016 - xfailed gcc.dg/20030612-1.c
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=16016
 */
/* { dg-do compile } */


// Subject: Re:  New: xfailed gcc.dg/20030612-1.c
// On Wed, 16 Jun 2004, rth at gcc dot gnu dot org wrote:
// <span class="quote">> Possibly what we intend is for statement expressions to end in an expression?
// > E.g. it's legal to end in "(void)0;" because that's an expression, but not
// > "return 3;" because that isn't?  If that's true, we should warn for *any* 
// > statement that's not an expression -- if, while, for, and so forth.  I have no
// > data as to how that affects code in the wild.</span >
// The documented semantics would be that if it ends in any nonexpression
// then it has type void (I don't know about the point of the warning).  
// "return 3" is another matter, as part of the question of whether it should
// be possible to jump out of statement expressions (<a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Statement expressions issues"
//    href="show_bug.cgi?id=772">bug 772</a>).  (Deprecating
// jumping into them would probably be safer than deprecating jumping out, as
// from past discussions it seems real code does jump out of them, and
// expects things to work when other parts of the expression have no
// side-effects.)


