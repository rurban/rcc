/* GCC Bug #119476 - [-Wdiscarded-qualifiers] False negative with -fplan9-extensions inheritance
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119476
 */
/* { dg-do compile } */


1:	struct foo {
                  3:	};
                  5:	struct bar {
                  7:	};
                 14:	{
                 22:	}


