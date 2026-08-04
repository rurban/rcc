/* GCC Bug #26613 - Corner case causes garbage to be output by -aux-info switch.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=26613
 */


// /* --- File foo.c --- */
void foo (struct { int a; int b; } * p)
{
}
// This causes the following output into foo.X:
// /* compiled from: . */
/* foo.c:3:NF */ extern void foo (struct { intint b; } *p); /* (p) struct { intint b; } *p; */
// As you can see, the type of the argument to foo is corrupted.
// The correct output is:
// /* compiled from: . */
/* foo.c:3:NF */ extern void foo (struct { int a; int b; } *p); /* (p) struct { int a; int b; } *p; */
// Fortunately, the fix is small, isolated, and low-risk.  You simply save a copy of the contents of the global variable before any recursion.  Since it's actually affecting me, I request that you apply the fix to 3.3, 3.4, 4.0, 4.1, and the trunk as soon as is feasible.
// I will be posting the patches (to the latest CVS) for 3.3, 3.4, 4.0, 4.1, and trunk to gcc-patches as soon as I'm done entering this.
// Thanks!
// Mark F. Haigh
// <a href="mailto:markfhaigh@sbcglobal.net">markfhaigh@sbcglobal.net</a>


