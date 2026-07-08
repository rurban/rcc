// Regression: a _Pragma(...) at file scope followed by ';' is an EMPTY
// declaration, not a type-less one. It must not break parsing nor emit a
// spurious "type defaults to int" warning. glibc's GCC_DIAG_IGNORE /
// GCC46_DIAG_IGNORE macros expand to exactly this (a _Pragma pair + trailing
// ';') under _FORTIFY_SOURCE, e.g. in gperf-generated tables
// (gortex: libredwg objects.in:46).

_Pragma("GCC diagnostic push");
_Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"");

// Parsing must continue correctly after the top-level _Pragma directives:
// this global and main() below must still be seen.
int global_after_pragma = 42;

_Pragma("GCC diagnostic pop");

int printf(const char *, ...);

int main(void)
{
    if (global_after_pragma != 42) return 1;
    printf("PASS\n");
    return 0;
}
