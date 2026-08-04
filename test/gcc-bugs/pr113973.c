/* GCC Bug #113973 - Please issue a warning when using plain character values in bitwise operations
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113973
 */


static char x = 0xD8;

int main(void)
{
    return 0x1200 | x;
}
// The value returned from main is 0xFFFFFFD8 on architectures with 32-bit int and signed characters by default. After just fixing a bug that was caused by an unexpected sign expansion when building an int from individual bytes, I'd rather have a warning if
// 1) A variable of type char is promoted to int.
// 2) The int value is used in an bitwise expression
// 3) More than 8 bits of the results are actually used
// 4) More than 8 bits may be non-zero

// Because of condition 3, this will yiels no warning on "char y = x | 0x40;" (top bits truncated, so condition 3 fails) and no warning on "int y = x & 0x40;" (all high bits are guaranteed to be zero, so condition 4 fails).
// The real-world bug that motivates this enhancement proposal is <a href="https://github.com/hfst/hfst-ospell/issues/43">https://github.com/hfst/hfst-ospell/issues/43</a>


