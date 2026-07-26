// Self-referential object macro: A must expand to B.A, not infinitely
struct BA { int A; };
struct BA B = { 42 };
#define A B.A

// Chained: control_points -> segx[r2].control_points -> paths[r1].segs[r2].control_points
struct CP { int val; };
struct Seg { struct CP *control_points; int num_cp; };
struct Path { struct Seg *segs; };
struct Obj { struct Path *paths; };

#define segx paths[r1].segs
#define control_points segx[r2].control_points

// A named GNU variadic parameter (args...) or __VA_ARGS__ reached through
// a ## paste, called with zero trailing arguments, must vanish entirely
// (not leak the literal parameter name into the output) — the kernel's
// __try_acquires##__VA_ARGS__##_ctx_lock(1, x) style relied on this.
#define GLUE_NAMED(x, args...) prefix_##args##_suffix
#define GLUE_VA(x, ...) prefix_##__VA_ARGS__##_suffix

int prefix__suffix(void) { return 11; }

// GNU extension: a conditional-compilation directive (#ifdef/#else/#endif)
// may appear in the middle of a function-like macro call's argument list —
// used by the kernel's struct_group()/__struct_group() (linux/stddef.h) to
// wrap some grouped-struct members behind config options, e.g.
// skbuff.h's `struct_group(headers, ... #ifdef CONFIG_NET_XGRESS ... #endif
// ...);`. The directive must be processed in place (still affecting which
// tokens are visible) while argument collection continues across it,
// rather than the whole macro call being abandoned and replayed as raw
// unexpanded text.
#define __struct_group(TAG, NAME, ATTRS, MEMBERS...) \
    union { \
        struct { MEMBERS } ATTRS; \
        struct TAG { MEMBERS } ATTRS NAME; \
    } ATTRS
#define struct_group(NAME, MEMBERS...) \
    __struct_group(/* no tag */, NAME, /* no attrs */, MEMBERS)

#define TEST_MACRO_GROUP_CONFIG_ON 1

struct group_test {
    int a;
    struct_group(headers,
        int b;
#ifdef TEST_MACRO_GROUP_CONFIG_ON
        int c;
#else
        int not_taken;
#endif
        int d;
    );
    int e;
};

int main() {
    if (A != 42) return 1;

    struct CP cp = { 7 };
    struct Seg seg = { &cp, 1 };
    struct Path path = { &seg };
    struct Obj obj = { &path };
    struct Obj *_obj = &obj;
    int r1 = 0, r2 = 0;
    if (_obj->control_points->val != 7) return 2;

    if (GLUE_NAMED(1,)() != 11) return 3;
    if (GLUE_VA(1,)() != 11) return 4;

    struct group_test g;
    g.headers.b = 1;
    g.c = 2;
    g.d = 3;
    g.e = 4;
    if (g.b != 1 || g.c != 2 || g.d != 3 || g.e != 4) return 5;

    return 0;
}
