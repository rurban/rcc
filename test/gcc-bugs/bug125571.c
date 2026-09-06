/* GCC Bug #125571 - compiling with gcc 380 times slower than clang (19 minutes vs 3 seconds)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125571
 */


/* Halving the number of if (setjmp) {} else {} blocks gives
 * 3/4s of the number of if (setjmp) {} else {} blocks gives
 * 7/8s of the number of if (setjmp) {} else {} blocks gives
 * cut the first 1/8s of the if (setjmp) {} else {} blocks */
#include <setjmp.h>
#include <stdio.h>

typedef unsigned char TRAIAN_TYPE_STEP;
TRAIAN_TYPE_STEP CAESAR_CONSTANT_PARAM__S_3_1;
TRAIAN_TYPE_STEP CAESAR_CONSTANT_PARAM__S_6_5;
jmp_buf CAESAR_SETJMP_BUFFER;

int main()
{
static FILE *CAESAR_RESULT;
CAESAR_RESULT = freopen ("/dev/null", "w", stdout);

#define X if ( _setjmp ( CAESAR_SETJMP_BUFFER) == 0) \
fprintf (CAESAR_RESULT,(((CAESAR_CONSTANT_PARAM__S_3_1) == (CAESAR_CONSTANT_PARAM__S_6_5))) ? "1\n" : "0\n"); \
else \
fprintf (CAESAR_RESULT,"2\n");

#define Y X X X X X X X X X X
#define Z Y Y Y Y Y Y Y Y Y Y
#define W Z Z Z Z Z Z Z Z Z Z
#define R W W W W W W W W W W
// R
// fclose(CAESAR_RESULT);
}


