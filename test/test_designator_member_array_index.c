/* A designated initializer chain that ends on an array-index step, e.g.
 * ".member[N] = value", was silently mis-parsed for FILE-SCOPE (global)
 * initializers. global_initializer()'s chain-designator loop handled two
 * kinds of steps — ".sub" (member access) and "[N]" (array index) — and
 * followed them one after another to compute a running byte offset, but
 * only the ".sub" step's code had the terminal check "is the chain over
 * (next token isn't '.' or '['), if so consume '=' and parse the value
 * here". The "[N]" step updated the offset/type and just did an
 * unconditional "continue" — so when a chain happened to *end* on an
 * array index (".extent[0] = ...", with no further ".member" after the
 * "]"), the while loop's own exit condition correctly saw the next token
 * was "=" and stopped iterating, but "= value" was never consumed by
 * anything: control fell out of the chain entirely, back to the outer
 * struct-initializer loop, which then tried to parse the literal token
 * "=" as if it were the start of the *next* member's initializer,
 * producing a string of confusing "expected specific operator" errors
 * (the local/non-global initializer path already handled this correctly,
 * by deferring "=" consumption until after the whole chain).
 *
 * Found via a real Linux kernel build: kernel/user.c's init_user_ns
 * definition initializes three uid/gid/projid maps whose
 * (anonymous-struct-wrapped) "extent" member is designated as
 * ".extent[0] = { .first = 0, .lower_first = 0, .count = 4294967295U }".
 */
struct extent { unsigned int first; unsigned int lower_first; unsigned int count; };
struct map { struct extent extent[2]; int nr_extents; };

/* Simplest case: chain ends on the array index with a scalar value. */
struct s1 { int arr[2]; int x; };
struct s1 g1 = {
    .arr[0] = 5,
    .x = 1,
};

/* The real kernel shape: chain ends on the array index with a nested
 * brace-initializer value. */
struct map g2 = {
    .extent[0] = { .first = 10, .lower_first = 20, .count = 30 },
    .nr_extents = 1,
};

/* Nested one level deeper, matching kernel/user.c's actual
 * "struct user_namespace { struct uid_gid_map uid_map; ... }" ->
 * "struct uid_gid_map { struct uid_gid_extent extent[N]; ... }" shape:
 * a designator chain of ".outer_member = { .extent[0] = {...}, ... }". */
struct wrapper { struct map m; };
struct wrapper g3 = {
    .m = {
        .extent[0] = {
            .first = 100,
            .lower_first = 200,
            .count = 300,
        },
        .nr_extents = 2,
    },
};

int main(void)
{
    if (g1.arr[0] != 5) return 1;
    if (g1.x != 1) return 2;

    if (g2.extent[0].first != 10) return 3;
    if (g2.extent[0].lower_first != 20) return 4;
    if (g2.extent[0].count != 30) return 5;
    if (g2.nr_extents != 1) return 6;

    if (g3.m.extent[0].first != 100) return 7;
    if (g3.m.extent[0].lower_first != 200) return 8;
    if (g3.m.extent[0].count != 300) return 9;
    if (g3.m.nr_extents != 2) return 10;

    return 0;
}
