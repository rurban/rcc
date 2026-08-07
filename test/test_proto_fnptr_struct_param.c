/* Prototype/definition redeclaration check (parser.c's function-definition
 * `equalc(tok, "{")` param-type comparison) must not false-positive when a
 * struct-typed parameter is nested inside a FUNCTION-POINTER parameter's
 * own parameter list, e.g. mruby's
 * `mrb_value mrb_get_values_at(..., mrb_value (*func)(mrb_state*, mrb_value, mrb_int))`.
 *
 * declarator_params() intentionally shallow-copies each parameter's Type
 * node (`*pt = *pty`) so the param-list node can carry its own name/
 * param_next without mutating the canonical tag's Type. For a struct/union
 * parameter that copy makes `pt` pointer-distinct from the canonical type
 * -- harmless at the top level (the prototype checker explicitly skips
 * struct/union params there), but type_equal() also recurses into nested
 * function-pointer parameter types, where the inner struct parameter gets
 * copied *again* by a second declarator_params() call. type_equal()'s
 * TY_STRUCT/TY_UNION case used raw pointer identity (`a == b`), which
 * fails for two independently-copied nodes of the same struct -- even
 * though they share the same canonical `members` list -- misdiagnosing a
 * real, unchanged prototype as "conflicting types". */

typedef struct value {
    int x;
} value;

value get_values_at(int argc, const value *argv,
                     value (*func)(int, value, int));

value
get_values_at(int argc, const value *argv, value (*func)(int, value, int))
{
    return func(argc, argv[0], 0);
}

static value
double_it(int argc, value v, int extra)
{
    (void)argc;
    (void)extra;
    v.x *= 2;
    return v;
}

int main(void) {
    value argv[1] = {{21}};
    value r = get_values_at(1, argv, double_it);
    return r.x == 42 ? 0 : 1;
}
