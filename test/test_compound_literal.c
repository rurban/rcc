/* Compound-literal regressions, consolidated from the former
 * test_compound_literal_{array,float,nested}.c, test_complit_designator_chain.c,
 * test_cast_addr_compound_literal.c and test_filescope_cl_static.c:
 *
 * - A parenthesized string literal — `("...")`, e.g. gettext's N_()
 *   — initializing a char-array member of a struct inside an array of
 *   such structs copies the literal's bytes, like the bare form
 *   (C11 6.7.9p14); it must not fall into the flat-array element path.
 */
#include <stdint.h>
#include <stdio.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

/* ---- float-member conversions (was test_compound_literal_float.c) ---- */

struct S { float a, b, c, d; };
typedef float f4 __attribute__((__vector_size__(16)));

static void test_float(void) {
    /* Control: plain brace initializer already converts int->float. */
    struct S ok = {1, 2, 3, 4};
    CHECK(ok.a == 1.0f && ok.b == 2.0f && ok.c == 3.0f && ok.d == 4.0f);

    /* Control: compound literal with float literals is correct. */
    struct S okf = (struct S){1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(okf.a == 1.0f && okf.b == 2.0f && okf.c == 3.0f && okf.d == 4.0f);

    /* Compound literal with int constants must convert to float. */
    struct S s = (struct S){1, 2, 3, 4};
    CHECK(s.a == 1.0f && s.b == 2.0f && s.c == 3.0f && s.d == 4.0f);

    /* Same for vector_size compound literals with int constants. */
    f4 v = (f4){1, 2, 3, 4};
    CHECK(v[0] == 1.0f && v[1] == 2.0f && v[2] == 3.0f && v[3] == 4.0f);
}

/* ---- nested designated initializers (was test_compound_literal_nested.c) ---- */

struct inner { int val; };
struct middle { struct inner in; };
struct outer { struct middle mid; int tag; };

struct iov_iter_like {
    union {
        void *ubuf;
        int idx;
    };
    int flags;
};

/* Mirrors qbe's struct Con: an enum "type" tag followed by a NAMED
 * (not anonymous) union "bits" -- ".bits.i" must resolve through the
 * union member's own offset within Con, not just "i"'s offset (0)
 * within the union itself. */
enum { CUndef, CBits, CAddr };
struct Con {
    int type;
    union { long long i; double d; } bits;
    char flt;
};

/* Compound literal used as an initializer (local_init_one's designator
 * chain) -- already correct before this fix; kept as a control case. */
static struct Con con_init_form(long long v) {
    struct Con c = { .type = CBits, .bits.i = v };
    return c;
}

/* Compound literal used as a plain EXPRESSION (the buggy path: parser.c's
 * unary()-level "(type){...}" designator-chain flattening). */
static struct Con con_expr_form(long long v) {
    return (struct Con){ .type = CBits, .bits.i = v };
}

static void test_nested(void) {
    /* Two levels of nested struct compound-literal brace init. */
    struct outer o = (struct outer){ { { 42 } }, 7 };
    CHECK(o.mid.in.val == 42);
    CHECK(o.tag == 7);

    /* A lone extra brace layer around a scalar leaf must still parse. */
    struct outer o2 = (struct outer){ .mid = { .in = { 99 } } };
    CHECK(o2.mid.in.val == 99);

    /* Designator reaching a member only visible through an anonymous
     * union must be resolved. */
    struct iov_iter_like it = (struct iov_iter_like){ .ubuf = (void *)0x1234, .flags = 1 };
    CHECK(it.ubuf == (void *)0x1234);
    CHECK(it.flags == 1);

    /* Explicit ".bits.i" designator through a NAMED union member: both
     * the plain-initializer form and the expression-context compound
     * literal must land the value at "bits"'s offset within Con, not
     * offset 0 (aliasing "type"). */
    struct Con c1 = con_init_form(-80);
    CHECK(c1.type == CBits);
    CHECK(c1.bits.i == -80);

    struct Con c2 = con_expr_form(24);
    CHECK(c2.type == CBits);
    CHECK(c2.bits.i == 24);

    /* Nested designated compound literal must apply the usual arithmetic
       conversions when a smaller constant is assigned to a wider member.
       A missing check_type() on the inner ND_ASSIGN previously stored a
       32-bit -1 into a long/long-long field without sign extension. */
    struct outer3 {
        union {
            struct { long long a; long long b; } s;
            int i;
        } u;
        int tag;
    };
    struct outer3 o3 = (struct outer3){ .u.s = { 1, -1 } };
    CHECK(o3.u.s.a == 1);
    CHECK(o3.u.s.b == -1);

    struct outer4 {
        union {
            struct { long size; long size_byte; char *data; } s;
            struct outer4 *next;
        } u;
        int tag;
    };
    char buf[64] = "hello";
    struct outer4 o4 = (struct outer4){ .u.s = { 5, -1, buf } };
    CHECK(o4.u.s.size == 5);
    CHECK(o4.u.s.size_byte == -1);
    CHECK(__builtin_strcmp(o4.u.s.data, "hello") == 0);
}

/* ---- array compound literals (was test_compound_literal_array.c) ---- */

typedef long wide_t;

struct value { int type; int x; };
struct instr { int opcode; struct value args[3]; };

struct range { int flags; int start; int end; };
struct info { struct range ranges[3]; };
static struct info the_info;

static void test_array(void) {
    /* Conversions: int constants assigned to long elements must
     * sign-extend, exactly like the nested designated struct case. */
    wide_t w[3] = (wide_t[]){ 1, -1, 0x7fffffff };
    CHECK(w[0] == 1L);
    CHECK(w[1] == -1L);
    CHECK(w[2] == 0x7fffffffL);

    /* Same through a pointer to the compound literal (lvalue form). */
    wide_t *wp = (wide_t[]){ 5, -1 };
    CHECK(wp[0] == 5L);
    CHECK(wp[1] == -1L);

    /* Chained ".args[0].type = val" designator inside a compound literal
     * used as a plain expression (address-taken argument). */
    const struct instr *p = &(const struct instr){
        .opcode = 9,
        .args[0].type = 10,
        .args[1].type = 20,
        .args[2].type = 30,
    };
    CHECK(p->opcode == 9);
    CHECK(p->args[0].type + p->args[1].type + p->args[2].type == 60);

    /* Range designator across the same chain shape. */
    const struct instr *p2 = &(const struct instr){
        .args[0 ... 2].type = 7,
    };
    CHECK(p2->args[0].type + p2->args[1].type + p2->args[2].type == 21);

    /* Struct's array member with braced struct elements at designated
     * indices (kernel execmem_info shape); undesignated elements must
     * stay zero. */
    the_info = (struct info){
        .ranges = {
            [0] = { .flags = 1, .start = 2, .end = 100 },
            [1] = { .flags = 2, .start = 3, .end = 200 },
        },
    };
    CHECK(the_info.ranges[0].flags == 1);
    CHECK(the_info.ranges[0].start == 2);
    CHECK(the_info.ranges[0].end == 100);
    CHECK(the_info.ranges[1].flags == 2);
    CHECK(the_info.ranges[1].start == 3);
    CHECK(the_info.ranges[1].end == 200);
    CHECK(the_info.ranges[2].flags == 0);
    CHECK(the_info.ranges[2].start == 0);
    CHECK(the_info.ranges[2].end == 0);

    /* Nested struct member that is itself an array, given as a plain
     * positional brace list one level deeper, plus a designated
     * single-index element. */
    struct params { int variant; int refs[3]; };
    struct op { int opcode; struct params parameters; };
    int a = 1, b = 2, c = 3;
    const struct op *o = &(const struct op){
        .opcode = 5,
        .parameters = {.variant = 8, .refs = {a, b, c}},
    };
    CHECK(o->opcode == 5);
    CHECK(o->parameters.variant == 8);
    CHECK(o->parameters.refs[0] + o->parameters.refs[1] + o->parameters.refs[2] == 6);

    const struct op *o2 = &(const struct op){
        .parameters = {.refs[1] = 99},
    };
    CHECK(o2->parameters.refs[1] == 99);
}

/* ---- designator chains through array indices (was test_complit_designator_chain.c) ---- */

typedef struct { const char* glsl_name; int type; int array_count; } uni;
typedef struct { int stage; int size; uni glsl_uniforms[4]; } ublock;
typedef struct { ublock uniform_blocks[4]; } shader;

typedef int vfmt;
typedef struct { vfmt format; int stride; } vattr;
typedef struct { vattr attrs[4]; int shader; } pipe;

/* union array with flat (brace-elided) elements */
typedef union U U;
union U { unsigned long long u64; long long i64; double number; void *pointer; };
typedef struct { U payload; int tag; } tstate_t;

static void *fake_addr(shader *s) { return (void *)s; }

static int test_designator_chain(void) {
    /* union array, flat elements: each element takes ONE value (C11
     * 6.7.9p13), the comma separates elements -- count_array_initializer's
     * flat-aggregate skip used to eat the comma after the single union
     * member, sizing the array as one element and choking on the leftover
     * "2" (janet's Janet[] = { ... } in ev.c). */
    tstate_t ts;
    ts.payload.u64 = 7;
    U upair[] = { ts.payload, ts.payload };
    U ipair[] = { 1, 2, 3 };
    /* designated array index with braced struct value, nested */
    shader s = *(const shader *)&(shader){
        .uniform_blocks[0] = {
            .stage = 1,
            .size = 16,
            .glsl_uniforms[0] = {
                .glsl_name = "vs_params",
                .type = 3,
                .array_count = 1,
            }
        },
        .uniform_blocks[2] = { .stage = 2 },
    };
    /* array member with braced, [N]-designated elements */
    shader s2 = *(const shader *)&(shader){
        .uniform_blocks = {
            [0] = {
                .stage = 4,
                .glsl_uniforms = {
                    [1] = { .glsl_name = "blob", .type = 2, .array_count = 1 },
                }
            },
            [3] = { .stage = 5 },
        },
    };
    /* chained designator through array index to leaf member */
    pipe p = *(const pipe *)&(pipe){
        .attrs[0].format = 7,
        .attrs[2].stride = 32,
        .shader = 0,
    };
    (void)fake_addr(&s);
    return (s.uniform_blocks[0].glsl_uniforms[0].array_count == 1 &&
            s.uniform_blocks[0].stage == 1 && s.uniform_blocks[2].stage == 2 &&
            s2.uniform_blocks[0].stage == 4 && s2.uniform_blocks[0].glsl_uniforms[1].array_count == 1 &&
            upair[0].u64 == 7 && upair[1].u64 == 7 &&
            ipair[0].u64 == 1 && ipair[1].u64 == 2 && ipair[2].u64 == 3 &&
            s2.uniform_blocks[3].stage == 5 &&
            p.attrs[0].format == 7 && p.attrs[2].stride == 32) ? 0 : 1;
}

/* ---- cast wrapping &(compound literal) (was test_cast_addr_compound_literal.c) ---- */

typedef struct {
    int type;
    double number;
} val_t;

struct entry {
    int type;
    void *p;
};

struct entry x = {2, (void *)&(val_t){1, 3.5}};

static int test_cast_addr(void) {
    val_t *v = x.p;
    if (v->type != 1 || v->number != 3.5) {
        fprintf(stderr, "FAIL: cast-prefixed &(compound literal) initializer broken\n");
        return 1;
    }
    return 0;
}

/* ---- file-scope static storage duration (was test_filescope_cl_static.c) ---- */

typedef struct {
    int a;
    int b;
} pair_t;

typedef struct {
    int name;
    uintptr_t value;
} entry_t;

/* Array context (matches the real njs shape). */
entry_t table[] = {
    {1, (uintptr_t)&(pair_t){10, 20}},
    {2, (uintptr_t)&(pair_t){30, 40}},
};

/* Single-struct context. */
entry_t single = {3, (uintptr_t)&(pair_t){50, 60}};

static int test_filescope(void) {
    pair_t *p0 = (pair_t *)table[0].value;
    pair_t *p1 = (pair_t *)table[1].value;
    pair_t *ps = (pair_t *)single.value;
    if (!p0 || p0->a != 10 || p0->b != 20) {
        fprintf(stderr, "FAIL: table[0].value broken\n");
        return 1;
    }
    if (!p1 || p1->a != 30 || p1->b != 40) {
        fprintf(stderr, "FAIL: table[1].value broken\n");
        return 2;
    }
    if (!ps || ps->a != 50 || ps->b != 60) {
        fprintf(stderr, "FAIL: single.value broken\n");
        return 3;
    }
    return 0;
}

/* ---- parenthesized string literals in struct members (was
 *      test_paren_strlit_struct_member.c) ---- */

#define N_(x) (x)

struct Format { unsigned long long factor; char string[9]; };

const struct Format formats[4][6] = {
  { { 1000ULL, N_("%.1f kB") }, { 1000000ULL, N_("%.1f MB") }, { 1000000000ULL, N_("%.1f GB") }, { 1000000000000ULL, N_("%.1f TB") }, { 1000000000000000ULL, N_("%.1f PB") }, { 1000000000000000000ULL, N_("%.1f EB") } },
  { { 1024ULL, N_("%.1f KiB") }, { 1048576ULL, N_("%.1f MiB") }, { 1073741824ULL, N_("%.1f GiB") }, { 1099511627776ULL, N_("%.1f TiB") }, { 1125899906842624ULL, N_("%.1f PiB") }, { 1152921504606846976ULL, N_("%.1f EiB") } },
  { { 1000ULL, N_("%.1f kb") }, { 1000000ULL, N_("%.1f Mb") }, { 1000000000ULL, N_("%.1f Gb") }, { 1000000000000ULL, N_("%.1f Tb") }, { 1000000000000000ULL, N_("%.1f Pb") }, { 1000000000000000000ULL, N_("%.1f Eb") } },
  { { 1024ULL, N_("%.1f Kib") }, { 1048576ULL, N_("%.1f Mib") }, { 1073741824ULL, N_("%.1f Gib") }, { 1099511627776ULL, N_("%.1f Tib") }, { 1125899906842624ULL, N_("%.1f Pib") }, { 1152921504606846976ULL, N_("%.1f Eib") } },
};

struct Row { char a[4]; char b[4]; };
struct Row rows[2][2] = { { { ("r0a"), ("r0b") }, { ("r0c"), ("r0d") } }, { { ("r1a"), ("r1b") }, { ("r1c"), ("r1d") } } };

static void test_paren_strlit(void) {
    CHECK(__builtin_strcmp(formats[0][0].string, "%.1f kB") == 0);
    CHECK(__builtin_strcmp(formats[0][5].string, "%.1f EB") == 0);
    CHECK(__builtin_strcmp(formats[1][1].string, "%.1f MiB") == 0);
    CHECK(__builtin_strcmp(formats[3][4].string, "%.1f Pib") == 0);
    CHECK(formats[0][0].factor == 1000ULL);
    CHECK(formats[3][5].factor == 1152921504606846976ULL);
    CHECK(__builtin_strcmp(rows[0][0].a, "r0a") == 0);
    CHECK(__builtin_strcmp(rows[0][1].b, "r0d") == 0);
    CHECK(__builtin_strcmp(rows[1][0].a, "r1a") == 0);
    CHECK(__builtin_strcmp(rows[1][1].b, "r1d") == 0);
}

int main(void) {
    int rc;
    test_float();
    test_nested();
    test_array();
    test_paren_strlit();
    if ((rc = test_designator_chain())) return rc;
    if ((rc = test_cast_addr())) return rc;
    if ((rc = test_filescope())) return rc;
    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
