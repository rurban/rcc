// Regression: a #line directive must retarget BOTH __LINE__ and __FILE__ (and
// the location the compiler uses in diagnostics). The preprocessor must pass
// the directive through so the line number AND file name are updated; the
// tokenizer must track the file per-token so it survives to later phases.
// This is what makes warnings in gperf output land on the .in source and
// warnings in system headers name the header, not the includer.

int strcmp(const char *, const char *);
int printf(const char *, ...);

int main(void)
{
#line 100 "phantom.c"
    int a = __LINE__;         /* the line after #line 100 -> 100 */
    int b = __LINE__;         /* 101 */
    const char *f = __FILE__; /* "phantom.c" */

    if (a != 100) { printf("FAIL a=%d\n", a); return 1; }
    if (b != 101) { printf("FAIL b=%d\n", b); return 2; }
    if (strcmp(f, "phantom.c") != 0) { printf("FAIL f=%s\n", f); return 3; }

    printf("PASS\n");
    return 0;
}
