/* SysV x86-64 small-aggregate return ABI: a union member containing a
 * float does NOT disqualify a <=16-byte struct from GP-register return.
 *
 * A union's eightbyte class is the MERGE of its members' classes, and
 * INTEGER dominates SSE in the merge (SysV psABI): a union holding both
 * an integer and a double (e.g. moar's MVMRegister) is INTEGER-class, so
 * a struct whose eightbytes are all such unions/scalars returns in
 * RAX:RDX with NO hidden pointer. rcc's type_is_all_integer() treated a
 * float leaf anywhere (including inside a union) as disqualifying, so it
 * passed a phantom hidden return pointer in RDI where a gcc-compiled
 * callee returned registers -- corrupting every argument after the
 * phantom pointer and reading garbage out of the (unwritten) buffer.
 *
 * Found via MoarVM's MVM_args_get_named_obj() (returns the 16-byte
 * MVMArgInfo, whose first member is the MVMRegister union of
 * {int64, double, ptr}): nqp's bootstrap segfaulted in
 * MVM_args_get_required_pos_str during bytecode deserialization.
 *
 * This test links a gcc-compiled callee (via dlopen) so the caller is
 * rcc-compiled and the callee is gcc-compiled -- the ABI boundary the
 * bug lives on. rcc-to-rcc calls agree on either convention and cannot
 * catch it.
 */
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

typedef union {
    long long o;
    double n64;
    void *s;
} MVMRegister;
typedef unsigned short MVMCallsiteEntry;
struct MVMArgInfo {
    MVMRegister arg;
    MVMCallsiteEntry flags;
    unsigned char exists;
    unsigned short arg_idx;
};

static const char helper_src[] =
    "typedef union { long long o; double n64; void *s; } R;\n"
    "typedef unsigned short E;\n"
    "struct MVMArgInfo { R arg; E flags; unsigned char exists; unsigned short arg_idx; };\n"
    "struct MVMArgInfo gcc_make_arginfo(int v) {\n"
    "    struct MVMArgInfo r;\n"
    "    r.arg.o = v; r.flags = 0; r.exists = 1; r.arg_idx = 7;\n"
    "    return r;\n"
    "}\n";

int main(void) {
#ifdef _WIN32
    /* Windows/MinGW: no dlopen() -- skip (ABI is uniform MinGW, not relevant) */
    printf("skipped\n");
    return 0;
#else
    /* Build the gcc-compiled callee shared object in /tmp. */
    const char *tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";
    char src_path[512], so_path[512], cmd[1024];
    snprintf(src_path, sizeof(src_path), "%s/rcc_arginfo_%d.c", tmp, (int)getpid());
    snprintf(so_path, sizeof(so_path), "%s/rcc_arginfo_%d.so", tmp, (int)getpid());
    FILE *f = fopen(src_path, "wb");
    if (!f) { perror("fopen src"); return 2; }
    fwrite(helper_src, 1, sizeof(helper_src) - 1, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "gcc -fPIC -shared -o %s %s 2>/dev/null", so_path, src_path);
    if (system(cmd) != 0) {
        printf("SKIP: no gcc available to build the cross-ABI callee\n");
        remove(src_path);
        return 0;
    }
    remove(src_path);

    void *h = dlopen(so_path, RTLD_NOW);
    remove(so_path);
    if (!h) {
        printf("SKIP: dlopen failed: %s\n", dlerror());
        return 0;
    }
    struct MVMArgInfo (*make_arginfo)(int) =
        (struct MVMArgInfo (*)(int))dlsym(h, "gcc_make_arginfo");
    if (!make_arginfo) {
        printf("FAIL: dlsym gcc_make_arginfo: %s\n", dlerror());
        dlclose(h);
        return 2;
    }

    /* The 4-arg call shape from moar:
     * rcc caller, gcc callee, 16-byte struct return. */
    struct MVMArgInfo a = make_arginfo(42);
    dlclose(h);

    if (a.arg.o != 42 || a.exists != 1 || a.arg_idx != 7) {
        printf("FAIL: gcc callee returned arg=%lld exists=%u idx=%u "
               "(expected 42/1/7) -- hidden-return-pointer ABI mismatch\n",
               (long long)a.arg.o, a.exists, a.arg_idx);
        return 1;
    }
    printf("OK: gcc callee 16-byte union-struct return via RAX:RDX\n");
    return 0;
#endif /* !_WIN32 */
}
