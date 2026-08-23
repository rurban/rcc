/* Compound-literal designated initializers with array-index steps in the
 * designator chain failed to parse ("expected specific operator"):
 *
 * 1. (T){ .m[0] = { ... } } — a struct compound literal whose array
 *    member is initialized at a designated index whose VALUE is itself a
 *    braced struct init (sokol's sg_shader_desc: ".uniform_blocks[0] = {
 *    ... .glsl_uniforms[0] = { ... } }"). The nested-element synthesizer
 *    (synth_struct_elem_literal) walked ".name" chains but stopped at an
 *    array-index step, leaving "[0]" for skip("=") to stumble over.
 * 2. (T){ .s = { .a[0].b = val } } — a chained member designator
 *    continuing through an array index into a leaf member
 *    (".attrs[0].format = val", sokol's sg_pipeline_desc).
 *    assign_nested_struct_init demanded "=" right after "]".
 *
 * Both now parse and generate the right assignments.
 */

typedef struct { const char* glsl_name; int type; int array_count; } uni;
typedef struct { int stage; int size; uni glsl_uniforms[4]; } ublock;
typedef struct { ublock uniform_blocks[4]; } shader;

typedef int vfmt;
typedef struct { vfmt format; int stride; } vattr;
typedef struct { vattr attrs[4]; int shader; } pipe;

static void* fake_addr(shader *s) { return (void*)s; }

int main(void) {
    /* shape 1: designated array index with braced struct value, nested */
    shader s = *(const shader*)&(shader){
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
    /* shape 3: array member with braced, [N]-designated elements */
    shader s2 = *(const shader*)&(shader){
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
    /* shape 2: chained designator through array index to leaf member */
    pipe p = *(const pipe*)&(pipe){
        .attrs[0].format = 7,
        .attrs[2].stride = 32,
        .shader = 0,
    };
    (void)fake_addr(&s);
    return (s.uniform_blocks[0].glsl_uniforms[0].array_count == 1 &&
            s.uniform_blocks[0].stage == 1 && s.uniform_blocks[2].stage == 2 &&
            s2.uniform_blocks[0].stage == 4 && s2.uniform_blocks[0].glsl_uniforms[1].array_count == 1 &&
            s2.uniform_blocks[3].stage == 5 &&
            p.attrs[0].format == 7 && p.attrs[2].stride == 32) ? 0 : 1;
}
