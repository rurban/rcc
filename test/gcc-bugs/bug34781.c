/* GCC Bug #34781 - __builtin_expect()'s first argument should be treated like other conditional expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=34781
 */
/* { dg-do compile } */


void func(void);

void test(int n) {
	if(n = 1)
		func();
	if(__builtin_expect(n = 1, 0))
		func();
}

// If __builtin_expect() is expected to be used outside of conditionals (the only useful case I can see is inside switch()), then the treatment of the argument should be done with knowledge of the surrounding context.


