/* GCC Bug #110519 - Optimize `for` loop that only assigns to a local variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110519
 */


struct symbol {
    struct symbol *next;
};

void f(const struct symbol *sym)
{
    for (const struct symbol *s = sym; s != (void *)0; s = s->next)
        do { } while (0);
}
// ~~~
// The above code was extracted from <<a href="https://github.com/NetBSD/src/blob/f74666e8d086952b27d76155223a5d497e69f5a8/usr.bin/xlint/lint1/decl.c#L1319">https://github.com/NetBSD/src/blob/f74666e8d086952b27d76155223a5d497e69f5a8/usr.bin/xlint/lint1/decl.c#L1319</a>>.
// Several other compilers know that this loop has no side effects, even at low optimization levels, see <<a href="https://godbolt.org/z/Y85EGcW3d">https://godbolt.org/z/Y85EGcW3d</a>>.


