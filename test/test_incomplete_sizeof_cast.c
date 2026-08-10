/* Two related C11 6.5.3.4p1 / 6.5.4p2 validation gaps found via ksh93's
 * `iffe` (its own configure-time probe generator): both probes rely on
 * the compiler correctly REJECTING invalid C to distinguish an opaque
 * (forward-declared, never-defined) struct from a real one, and rcc was
 * silently accepting both invalid forms instead.
 *
 * 1. `sizeof` on an incomplete struct/union type (or an expression whose
 *    type is one) must be a hard compile error -- rcc's `sizeof`
 *    handling just read the type's `->size` field directly (0 for an
 *    incomplete type) and returned that, instead of rejecting the
 *    expression outright.
 *
 * 2. A cast's type name must be void or scalar, and (unless the target
 *    is void) the operand must also be scalar -- struct/union values
 *    are never castable to or from anything but void. rcc's cast
 *    parsing built the ND_CAST node unconditionally with no such check
 *    at all, silently letting a struct's raw bytes get reinterpreted as
 *    an integer.
 *
 * Both are exercised here as standalone subprocess compiles (the
 * invalid forms must fail with a nonzero exit; this test doesn't care
 * about the exact diagnostic text, only that rcc rejects them like gcc
 * does) plus positive compiles proving the surrounding, GCC-tolerated
 * leniencies this fix must NOT break:
 *   - a cast to the exact same struct type as the operand (a no-op
 *     identity cast GCC tolerates even though the target isn't
 *     scalar);
 *   - a cast to a union type from one of its own member's types (the
 *     documented GNU "cast to union" extension);
 *   - an array decaying to a pointer before a cast to an integer type
 *     (e.g. `(long)some_array`) -- array values are never aggregates
 *     from a cast's point of view, they're pointers by the time a cast
 *     sees them;
 *   - `sizeof` and casts on a genuinely complete struct, which must
 *     keep working exactly as before.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int compile_ok(const char *rcc, const char *td, int pid, const char *src) {
    char path[600], obj[700], cmd[2400];
    snprintf(path, sizeof(path), "%s/t_incsz_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/t_incsz_%d.o", td, pid);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(src, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, path, obj);
    int rc = system(cmd);
    remove(path);
    remove(obj);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    /* 1a. sizeof(type-name) on an opaque (never-defined) struct: reject. */
    if (compile_ok(rcc, td, pid,
            "typedef struct opaque OPAQUE;\n"
            "static OPAQUE i;\n"
            "int n = sizeof(OPAQUE);\n") == 0) {
        printf("FAIL: sizeof(OPAQUE) (incomplete type-name) compiled, should be rejected\n");
        return 1;
    }

    /* 1b. sizeof(expr) whose type is an opaque struct: reject too (the
     * exact iffe probe shape -- `static OPAQUE i; sizeof(i)`, a plain
     * identifier operand, not a type-name). */
    if (compile_ok(rcc, td, pid,
            "typedef struct opaque OPAQUE;\n"
            "static OPAQUE i;\n"
            "int n = sizeof(i);\n") == 0) {
        printf("FAIL: sizeof(i) where i has incomplete type compiled, should be rejected\n");
        return 1;
    }

    /* 1c. sizeof on a genuinely complete struct still works. */
    if (compile_ok(rcc, td, pid,
            "struct full { int pad; };\n"
            "static struct full i;\n"
            "int n = sizeof(i);\n"
            "int m = sizeof(struct full);\n") != 0) {
        printf("FAIL: sizeof on a complete struct was rejected\n");
        return 1;
    }

    /* 2a. Casting a struct value to a scalar type: reject (the second
     * iffe probe -- `(unsigned long)i` where i is a real, complete
     * struct; iffe uses this specifically to catch a struct being
     * treated as reinterpretable, which real C never allows). */
    if (compile_ok(rcc, td, pid,
            "struct full { int pad; };\n"
            "static struct full i;\n"
            "unsigned long f(void) { return (unsigned long)i; }\n") == 0) {
        printf("FAIL: (unsigned long)struct_value compiled, should be rejected\n");
        return 1;
    }

    /* 2b. Casting a scalar to a (non-union) struct type: reject. */
    if (compile_ok(rcc, td, pid,
            "struct full { int pad; };\n"
            "struct full f(long x) { return (struct full)x; }\n") == 0) {
        printf("FAIL: (struct full)long_value compiled, should be rejected\n");
        return 1;
    }

    /* 2c. Casting between two distinct, differently-named struct types
     * (even with identical member layout) is still rejected -- only a
     * cast to the operand's OWN type is the tolerated no-op below. */
    if (compile_ok(rcc, td, pid,
            "struct A { int x, y; };\n"
            "struct B { int x, y; };\n"
            "struct A f(struct B v) { return (struct A)v; }\n") == 0) {
        printf("FAIL: (struct A)struct_B_value compiled, should be rejected\n");
        return 1;
    }

    /* 2d. GCC-tolerated leniency: casting a value to its OWN struct type
     * is a no-op identity cast, not an error. */
    if (compile_ok(rcc, td, pid,
            "struct full { int pad; };\n"
            "struct full f(struct full v) { return (struct full)v; }\n") != 0) {
        printf("FAIL: (struct full)struct_full_value (same type) was rejected\n");
        return 1;
    }

    /* 2e. GNU "cast to union type" extension: casting a value whose type
     * matches one of the union's own members must still work. */
    if (compile_ok(rcc, td, pid,
            "typedef union { unsigned long long d; struct { unsigned int l, h; } s; } U;\n"
            "U f(unsigned long long x) { return (U)x; }\n") != 0) {
        printf("FAIL: (union U)matching_member_type was rejected\n");
        return 1;
    }

    /* 2f. ... but casting a value whose type matches NONE of the
     * union's members is still rejected. */
    if (compile_ok(rcc, td, pid,
            "typedef union { unsigned long long d; } U;\n"
            "U f(double x) { return (U)x; }\n") == 0) {
        printf("FAIL: (union U)non_member_type compiled, should be rejected\n");
        return 1;
    }

    /* 2g. An array decays to a pointer before a cast -- casting an
     * array to an integer type must still work (it's a pointer by the
     * time the cast sees it, never treated as an aggregate operand). */
    if (compile_ok(rcc, td, pid,
            "long arr[4];\n"
            "long f(void) { return (long)arr; }\n") != 0) {
        printf("FAIL: (long)array_value (pointer decay) was rejected\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}
