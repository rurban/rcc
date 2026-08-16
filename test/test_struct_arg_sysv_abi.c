/* SysV x86-64 struct-BY-VALUE ARGUMENT passing: rcc used the Win64
 * by-reference convention ("any struct/union argument >8 bytes is a
 * hidden pointer in a GP register or one stack slot") unconditionally on
 * BOTH platforms. This is a real ABI convention (self-consistent for
 * rcc-to-rcc calls, which is why `make check-all`'s entirely
 * self-compiled test corpus never caught it), but Linux/SysV's real ABI
 * requires:
 *   - <=16 bytes, entirely INTEGER-class: raw VALUE in up to 2
 *     consecutive GP registers (or 2 raw stack slots on overflow) --
 *     never a pointer.
 *   - >16 bytes: unconditionally MEMORY class -- raw bytes copied
 *     directly onto the stack, consuming zero GP registers regardless of
 *     availability -- never a pointer, never a register.
 * Found via test_tomlc17's real `simplecpp` (g++-compiled) calling
 * rcc-compiled `tomlc17.o`'s `toml_seek(toml_datum_t table, ...)` --
 * `table` is a 40-byte struct, silently corrupted crossing the ABI
 * boundary. Reproduced here directly against the system `cc`, since
 * `make check-all`'s rcc-to-rcc self-consistency can never catch a
 * caller/callee ABI-convention MISMATCH (rcc's own convention was
 * consistent with itself, just not with `cc`).
 *
 * Fixed in codegen.c (gen_funcall's SysV classification/marshalling
 * loops, both function-parameter prologue passes, and va_arg's own
 * struct-argument read-back) to implement the real convention above.
 * Along the way, two narrower stacked bugs surfaced and were fixed in
 * the same pass:
 *   - the tail ("hi") eightbyte's read/write size for a struct sized
 *     9-15 bytes used `chunk >= 8 ? 8 : 4`, silently dropping real data
 *     bytes for a remainder of 5, 6, or 7 (only reading/writing 4 of
 *     them) -- fixed to `chunk <= 4 ? 4 : 8` (always covers every real
 *     byte, only ever safely *over*-reads into padding).
 *   - `__m128`/`__m128i`/`__m128d` (SSE vector types) are internally
 *     represented as `TY_STRUCT` with `is_vector` set; the new
 *     classification wrongly captured `__m128i` (all-integer 16-byte
 *     members) as "2 GP registers" instead of leaving it on the
 *     pre-existing (correct) vector-argument path -- fixed by excluding
 *     `is_vector` types from every new classification branch.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "test_common.h"

static int write_file_helper(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

static int run_compile(const char *cc, const char *flags, const char *src, const char *obj) {
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT, cc, flags, src, obj);
    return system(cmd);
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

#ifdef _WIN32
    // Win64's by-reference convention for structs >8 bytes IS this
    // fix's baseline (unchanged) -- cases 1-2 (raw-value SysV classes)
    // don't apply here; case 3 below still runs (pure rcc, portable).
    (void)rcc; (void)td; (void)pid;
#elif defined(__APPLE__)
    // Darwin's own ABI/toolchain differs enough (and this session's fix
    // was verified only against Linux SysV + a real system `cc`); skip
    // cases 1-2 rather than risk a false failure on an unverified
    // platform. Case 3 below still runs (pure rcc, portable).
    (void)rcc; (void)td; (void)pid;
#elif defined(__aarch64__)
    // Cases 1-2 link a real system `cc`-compiled caller against an
    // rcc-compiled callee to prove genuine cross-compiler ABI
    // compatibility -- meaningless (and actively wrong) when this test
    // binary itself was cross-compiled for aarch64 and runs under
    // qemu-aarch64: `cc` found via PATH there resolves to the HOST's
    // native x86-64 compiler, not a real aarch64 one, so linking its
    // output against an aarch64 callee would just fail on an
    // architecture mismatch unrelated to this fix. Case 3 (pure rcc,
    // no external cc) still verifies the ARM64 codegen path fully.
    (void)rcc; (void)td; (void)pid;
#else
    char callee_src[600], callee_obj[700], caller_src[600], caller_obj[700], exe[700];

    // Case 1: >16-byte struct argument (MEMORY class -- must be raw
    // stack bytes, never a pointer). Matches tomlc17's real
    // `toml_datum_t` (40 bytes) exactly in shape (mixed pointer/int64
    // union member forcing the union, hence the struct, past 16 bytes).
    snprintf(callee_src, sizeof(callee_src), "%s/test_sabi1_callee_%d.c", td, pid);
    snprintf(callee_obj, sizeof(callee_obj), "%s/test_sabi1_callee_%d.o", td, pid);
    if (!write_file_helper(callee_src,
            "typedef struct { long long a,b,c,d,e; } Big40;\n"
            "long long sabi_sum_big(Big40 x) { return x.a+x.b+x.c+x.d+x.e; }\n"))
        return 1;
    if (run_compile(rcc, "-O0", callee_src, callee_obj) != 0) {
        printf("FAIL: case 1 callee failed to compile\n");
        return 2;
    }
    snprintf(caller_src, sizeof(caller_src), "%s/test_sabi1_caller_%d.c", td, pid);
    snprintf(caller_obj, sizeof(caller_obj), "%s/test_sabi1_caller_%d.o", td, pid);
    if (!write_file_helper(caller_src,
            "typedef struct { long long a,b,c,d,e; } Big40;\n"
            "extern long long sabi_sum_big(Big40 x);\n"
            "int main(void) { Big40 b = {1,2,3,4,5}; return sabi_sum_big(b) == 15 ? 0 : 1; }\n"))
        return 3;
    if (run_compile("cc", "-O0", caller_src, caller_obj) != 0) {
        printf("FAIL: case 1 caller (real cc) failed to compile\n");
        return 4;
    }
    snprintf(exe, sizeof(exe), "%s/test_sabi1_exe_%d", td, pid);
    char link_cmd[2400];
    snprintf(link_cmd, sizeof(link_cmd), "cc -o %s %s %s " NULL_REDIRECT, exe, caller_obj, callee_obj);
    if (system(link_cmd) != 0) {
        printf("FAIL: case 1 link failed\n");
        return 5;
    }
    char run_cmd[800];
    snprintf(run_cmd, sizeof(run_cmd), "%s", exe);
    int rc1 = system(run_cmd);
    remove(callee_src); remove(callee_obj);
    remove(caller_src); remove(caller_obj); remove(exe);
    if (rc1 != 0) {
        printf("FAIL: case 1 (>16-byte struct arg, cc caller / rcc callee) "
               "returned wrong value (rc=%d)\n", rc1);
        return 6;
    }

    // Case 2: 9-16 byte all-integer struct argument (raw VALUE in 2 GP
    // registers, never a pointer).
    snprintf(callee_src, sizeof(callee_src), "%s/test_sabi2_callee_%d.c", td, pid);
    snprintf(callee_obj, sizeof(callee_obj), "%s/test_sabi2_callee_%d.o", td, pid);
    if (!write_file_helper(callee_src,
            "typedef struct { long long a; int b; } Mid12;\n"
            "long long sabi_sum_mid(Mid12 x) { return x.a + x.b; }\n"))
        return 7;
    if (run_compile(rcc, "-O0", callee_src, callee_obj) != 0) {
        printf("FAIL: case 2 callee failed to compile\n");
        return 8;
    }
    snprintf(caller_src, sizeof(caller_src), "%s/test_sabi2_caller_%d.c", td, pid);
    snprintf(caller_obj, sizeof(caller_obj), "%s/test_sabi2_caller_%d.o", td, pid);
    if (!write_file_helper(caller_src,
            "typedef struct { long long a; int b; } Mid12;\n"
            "extern long long sabi_sum_mid(Mid12 x);\n"
            "int main(void) { Mid12 m = {100, 23}; return sabi_sum_mid(m) == 123 ? 0 : 1; }\n"))
        return 9;
    if (run_compile("cc", "-O0", caller_src, caller_obj) != 0) {
        printf("FAIL: case 2 caller (real cc) failed to compile\n");
        return 10;
    }
    snprintf(exe, sizeof(exe), "%s/test_sabi2_exe_%d", td, pid);
    snprintf(link_cmd, sizeof(link_cmd), "cc -o %s %s %s " NULL_REDIRECT, exe, caller_obj, callee_obj);
    if (system(link_cmd) != 0) {
        printf("FAIL: case 2 link failed\n");
        return 11;
    }
    snprintf(run_cmd, sizeof(run_cmd), "%s", exe);
    int rc2 = system(run_cmd);
    remove(callee_src); remove(callee_obj);
    remove(caller_src); remove(caller_obj); remove(exe);
    if (rc2 != 0) {
        printf("FAIL: case 2 (9-16-byte all-integer struct arg, cc caller "
               "/ rcc callee) returned wrong value (rc=%d)\n", rc2);
        return 12;
    }
#endif

    char callee_src3[600], exe3[700];

    // Case 3: a 13-byte struct (tail eightbyte remainder = 5 bytes,
    // exactly the `chunk >= 8 ? 8 : 4` byte-loss regression) forced to
    // overflow past the 6 GP argument registers onto the stack, so the
    // caller-side stack-overflow raw-byte-copy path is exercised too
    // -- rcc compiles BOTH sides here (matching the real GCC-torture
    // va-arg-22 regression's own shape), verified end to end. Runs on
    // every platform (pure rcc, no external cc).
    snprintf(callee_src3, sizeof(callee_src3), "%s/test_sabi3_%d.c", td, pid);
    if (!write_file_helper(callee_src3,
            "typedef struct { char x[13]; } A13;\n"
            "int sabi_sum13(int a,int b,int c,int d,int e,int f, A13 s) {\n"
            "  int t = a+b+c+d+e+f;\n"
            "  for (int i = 0; i < 13; i++) t += s.x[i];\n"
            "  return t;\n"
            "}\n"
            "int main(void) {\n"
            "  A13 s; for (int i = 0; i < 13; i++) s.x[i] = i + 1;\n"
            "  return sabi_sum13(1,2,3,4,5,6, s) == 21 + 91 ? 0 : 1;\n"
            "}\n"))
        return 13;
    snprintf(exe3, sizeof(exe3), "%s/test_sabi3_exe_%d", td, pid);
    char full_cmd[1400];
    snprintf(full_cmd, sizeof(full_cmd), "%s -O0 -o %s %s " NULL_REDIRECT, rcc, exe3, callee_src3);
    if (system(full_cmd) != 0) {
        printf("FAIL: case 3 failed to compile\n");
        return 14;
    }
    char run_cmd3[800];
    snprintf(run_cmd3, sizeof(run_cmd3), "%s", exe3);
    int rc3 = system(run_cmd3);
    remove(callee_src3);
    remove(exe3);
    if (rc3 != 0) {
        printf("FAIL: case 3 (13-byte struct, tail-eightbyte byte-loss "
               "regression, stack-overflow arg) returned wrong value "
               "(rc=%d)\n", rc3);
        return 15;
    }

    printf("OK\n");
    return 0;
}
