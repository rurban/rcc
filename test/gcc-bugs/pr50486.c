/* GCC Bug #50486 - Missed -Wsign-conversion with signed -> unsigned casting and enums
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50486
 */


enum e {
//         A,
//         B,
//         C
};

int a(enum e e)
{
        if (e < 0)
                return 1;
        return 0;
}

int b(void)
{
        return a(-1);
}
// Since enum e has only positive values GCC states that enum is unsigned and eliminates the if-check. I can see warning about it with -Wtype-limits.
// But GCC generates no warning at a(-1), so it silently casts -1 to unsigned.


