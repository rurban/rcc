/* GCC Bug #61939 - warn when __attribute__((aligned(x))) is ignored
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61939
 */


void copy_something(void *p, const void *s) {
    struct some_struct __aligned__((aligned(8))) *_d = d;
    struct some_struct __aligned__((aligned(8))) *_s = s;
//     *_d = *_s;
}

// However, it continually generate copy code with an alignment prologue and epilogue. Then I learned (on freenode's ##gcc) that the proper way to tell gcc this was via __builtin_assume_aligned() and my problem was solved:

void copy_something(void *p, const void *s) {
    struct some_struct *_d = __builtin_assume_aligned(d, 8);
    struct some_struct *_s = __builtin_assume_aligned(s, 8);
//     *_d = *_s;
}
// It would seem to me that gcc should issue a warning when such an attribute is assigned, but has no effect as it does in some other cases. This seems to apply to all cases where it is used to define a type, of which you derive a pointer, e.g.:
int __aligned__((aligned(1))) *i;
// Here, I am actually expecting to get a pointer that I can safely access ints that are not aligned to the machine word, but will indeed blow up on machines that do not allow unaligned access of words.  The quandary here is that simply treating it as an array of bytes is less efficient on x86, where an unaligned 32-bit mov would be faster than a rep movsb, so such a request is often highly reasonable.


