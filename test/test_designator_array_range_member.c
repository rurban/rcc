/* GNU C designated array-range initializer (".field[LOW ... HIGH] = val")
 * directly on a struct member array was rejected with "expected specific
 * operator" -- parser.c's chain-designator loops in global_init_one()
 * (top-level and static/constexpr locals) and local_init_one() (auto
 * locals) each parsed the "[N]" designator step as a single constant
 * index and had no handling for a trailing "..." range form, unlike the
 * *top-level* array-with-braces path a few lines above, which already
 * supported "[LOW ... HIGH] = val" for a bare array target.
 *
 * Found via test/third_party's opcodes/i386-dis.c (binutils) and
 * coreutils' lib/utimecmp.c, both of which use range designators on a
 * struct member array field.
 */
struct flags { int f[8]; int x; };

/* File-scope / static: goes through global_init_one(). */
struct flags g = { .f[0 ... 3] = 7, .x = 9 };
static struct flags s_static = { .f[2 ... 4] = 3, .x = 1 };

int check_static_local(void)
{
    static struct flags loc = { .f[1 ... 5] = 11, .x = 22 };
    if (loc.f[0] != 0) return 1;
    if (loc.f[1] != 11 || loc.f[2] != 11 || loc.f[3] != 11 ||
        loc.f[4] != 11 || loc.f[5] != 11)
        return 2;
    if (loc.f[6] != 0 || loc.f[7] != 0) return 3;
    if (loc.x != 22) return 4;
    return 0;
}

/* Auto local: goes through local_init_one(). */
int check_auto_local(void)
{
    struct flags loc = { .f[0 ... 2] = 5, .x = 8 };
    if (loc.f[0] != 5 || loc.f[1] != 5 || loc.f[2] != 5) return 1;
    if (loc.f[3] != 0 || loc.f[7] != 0) return 2;
    if (loc.x != 8) return 3;
    return 0;
}

/* Side-effecting value: must be evaluated exactly once, not once per
 * covered index. */
int calls;
int side_effect(void) { calls++; return 42; }

int check_side_effect_once(void)
{
    struct flags loc = { .f[0 ... 3] = side_effect() };
    if (calls != 1) return 1;
    if (loc.f[0] != 42 || loc.f[3] != 42) return 2;
    return 0;
}

int main(void)
{
    if (g.f[0] != 7 || g.f[3] != 7) return 1;
    if (g.f[4] != 0 || g.f[7] != 0) return 2;
    if (g.x != 9) return 3;

    if (s_static.f[1] != 0 || s_static.f[2] != 3 || s_static.f[4] != 3 ||
        s_static.f[5] != 0)
        return 4;
    if (s_static.x != 1) return 5;

    int r = check_static_local();
    if (r) return 10 + r;

    r = check_auto_local();
    if (r) return 20 + r;

    r = check_side_effect_once();
    if (r) return 30 + r;

    return 0;
}
