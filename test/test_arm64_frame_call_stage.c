// ARM64: a nested call's argument-staging temp slot must fall back to
// indirect (scratch-register) addressing once its frame-pointer-relative
// offset exceeds STUR/LDUR's 9-bit signed immediate range (-256..255),
// instead of letting the raw immediate silently wrap.
//
// codegen_asm.h's asm_stur_fp()/asm_ldur_fp() (used by gen_funcall()'s
// register-pressure argument-staging path, spill_offset() register
// spills, and a few libgcc-helper call sites) computed `stur x{r},
// [x29, #-off]` unconditionally, with no range check on `off`. AArch64's
// STUR/LDUR encode a signed 9-bit immediate (imm9, range -256..255);
// arm64_stur()/arm64_ldur() mask the raw value with `& 0x1ff` with no
// bounds validation, so an out-of-range offset (e.g. -264) wraps to a
// small POSITIVE offset instead (-264 & 0x1ff == +248 in the encoded
// field) -- silently addressing memory ABOVE the frame pointer instead
// of the intended slot below it.
//
// This stayed dormant for small/shallow functions (where `off` -- the
// sum of the function's own parameter/local storage plus however many
// 8-byte temps a nested call's argument marshaling needs -- never
// exceeds 255) and only manifests once a function's own storage is deep
// enough that adding a handful of staged call-argument temps pushes past
// that boundary. A function receiving several small non-HFA struct
// parameters (plain GP registers) interleaved with several HFA struct
// parameters (SIMD/FP registers, each needing its own frame slot) easily
// reaches that depth, and then calling a variadic function (like this
// test's own printf-shaped call, needing several staged register-
// pressure temps of its own) reaches into the corrupted-offset range --
// writing float/double bit patterns from the callee's own arguments
// directly over the region just above the frame, up to and including
// the CALLER's saved x29/x30 a few slots further up, corrupting the
// return address itself (a SIGSEGV at a floating-point-bit-pattern
// "instruction address", not a wrong-value bug).
//
// Found via GCC c-testsuite's 00204.c (ARM64 calling-convention test):
// `fa4()`'s exact parameter shape (three tiny char-array structs
// interleaved with three float/double/long-double HFA structs) hit this
// after the small-struct-return-ABI investigation but was still failing
// with a SIGSEGV; bisected via a qemu-aarch64 gdbstub trace showing the
// crash PC and X29/X30 holding literal IEEE-754 bit patterns for 34.1/
// 34.3 (the struct's own long-double member values) instead of valid
// code/stack addresses.

#include <stdio.h>
#include <string.h>

struct s1 { char x[1]; } s1 = { "0" };
struct hfa_f4 { float a, b, c, d; } hfa_f4 = { 14.1f, 14.2f, 14.3f, 14.4f };
struct s2 { char x[2]; } s2 = { "12" };
struct hfa_d4 { double a, b, c, d; } hfa_d4 = { 24.1, 24.2, 24.3, 24.4 };
struct s3 { char x[3]; } s3 = { "345" };
struct hfa_l4 { long double a, b, c, d; } hfa_l4 = { 34.1L, 34.2L, 34.3L, 34.4L };
char result[128];

// Deep enough own-frame storage (three GP-passed structs, three HFA
// structs each needing its own multi-member frame slot) that the nested
// snprintf() call's own argument-staging temps push past the STUR/LDUR
// 9-bit immediate's -256 lower bound. Parameter shape matters: adding
// or removing parameters shifts every later local's frame offset, so
// this mirrors the exact shape (and global, not local, operands) that
// reproduced the bug -- don't "clean up" by passing an output
// buffer/size as extra parameters, that shifts the layout enough to
// stop reproducing it.
static void deep_frame_call(struct s1 a, struct hfa_f4 b, struct s2 c,
                             struct hfa_d4 d, struct s3 e, struct hfa_l4 f)
{
    snprintf(result, sizeof(result),
             "%.1s %.1f %.1f %.2s %.1f %.1f %.3s %.1Lf %.1Lf",
             a.x, b.a, b.d, c.x, d.a, d.d, e.x, f.a, f.d);
}

int main(void)
{
    deep_frame_call(s1, hfa_f4, s2, hfa_d4, s3, hfa_l4);

    const char *expect = "0 14.1 14.4 12 24.1 24.4 345 34.1 34.4";
    if (strcmp(result, expect) != 0) {
        fprintf(stderr, "got:      %s\nexpected: %s\n", result, expect);
        return 1;
    }
    return 0;
}
