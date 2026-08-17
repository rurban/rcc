/* A designated initializer chain that continues past an array-index step
 * with a further ".member" designator on a struct/union-typed array
 * element -- e.g. ".args[0].type = val" -- was unsupported specifically
 * inside an EXPRESSION-CONTEXT compound literal (a "(Type){...}" used as
 * a plain expression, as opposed to a whole variable's own initializer,
 * which already handled this correctly via local_init_one()/
 * global_init_one()). Two independent bugs, both found via kefir (a real,
 * actively maintained C11 compiler):
 *
 *  1. The array-element designator loop always demanded "=" directly
 *     after "]", so ".args[0].type = val" hit "expected specific
 *     operator" on the "." -- kefir's DEF_OPCODE_* macros
 *     (source/codegen/amd64/asmcmp.c) build hundreds of functions this
 *     way: `&(const struct kefir_asmcmp_instruction) {.opcode = ...,
 *     .args[0].type = ..., ...}`.
 *  2. Once the chain was parsed, the synthesized member-access node's
 *     ND_DEREF sub-node never had check_type() called on it (every other
 *     call site immediately following a fresh ND_DEREF construction does)
 *     -- its ->ty stayed NULL, segfaulting codegen's gen_addr/gen at the
 *     first attempt to inspect it.
 *
 * A third, closely related bug in the same expression-context compound
 * literal parser: a NESTED struct member that is itself an array, given
 * as its own plain brace-enclosed initializer (positional and/or [N]=val
 * designated elements) one level deeper than the top-level array-member
 * case -- e.g. kefir's own
 * ".parameters = {.condition_variant = v, .refs = {a, b, c}}"
 * (source/optimizer/builder.c) -- fell through to a generic "lone extra
 * brace around a scalar" path that only ever consumed one element,
 * leaving the rest of the brace list unparsed ("expected specific
 * operator" on the next element).
 */
struct value { int type; int x; };
struct instr { int opcode; struct value args[3]; };

static int sum_types(const struct instr *i) {
    return i->args[0].type + i->args[1].type + i->args[2].type;
}

struct params { int variant; int refs[3]; };
struct op { int opcode; struct params parameters; };

static int sum_refs(const struct op *o) {
    return o->parameters.refs[0] + o->parameters.refs[1] + o->parameters.refs[2];
}

int main(void)
{
    /* Bug 1+2: chained ".member[idx].submember = value" inside a local,
     * address-taken compound literal expression (function argument, not
     * a variable initializer). */
    const struct instr *p = &(const struct instr){
        .opcode = 9,
        .args[0].type = 10,
        .args[1].type = 20,
        .args[2].type = 30,
    };
    if (p->opcode != 9) return 1;
    if (sum_types(p) != 60) return 2;

    /* Range-designator variant across the same chain shape. */
    const struct instr *p2 = &(const struct instr){
        .args[0 ... 2].type = 7,
    };
    if (sum_types(p2) != 21) return 3;

    /* Bug 3: nested struct member that is itself an array, given as a
     * plain positional brace list one level deeper. */
    int a = 1, b = 2, c = 3;
    const struct op *o = &(const struct op){
        .opcode = 5,
        .parameters = {.variant = 8, .refs = {a, b, c}},
    };
    if (o->opcode != 5) return 4;
    if (o->parameters.variant != 8) return 5;
    if (sum_refs(o) != 6) return 6;

    /* Same nested-array-member shape with an explicit designated index. */
    const struct op *o2 = &(const struct op){
        .parameters = {.refs[1] = 99},
    };
    if (o2->parameters.refs[1] != 99) return 7;

    return 0;
}
