/* A struct compound literal's ARRAY member, initialized with designated
 * indices whose values are themselves braced struct/union initializers,
 * must parse each element's braces as a nested compound literal -- not
 * fall through to assign(), which can't parse a bare "{...}" as an
 * expression at all.
 *
 * Regression: the top-level array-compound-literal path already handled
 * "{.member=val,...}" as a struct/union array element (synthesizing a
 * nested compound literal for it), but a struct compound literal's own
 * array-typed *member* used a separate code path that called assign()
 * unconditionally on a designated index's value, never checking whether
 * the array's element type needed the same treatment.
 *
 * Found via a real Linux kernel build: arch/x86/mm/init.c's
 *   execmem_info = (struct execmem_info){
 *       .ranges = {
 *           [EXECMEM_MODULE_TEXT] = {
 *               .flags = flags, .start = start, .end = MODULES_END, ...
 *           },
 *           ...
 *       },
 *   };
 */

enum { RANGE_A, RANGE_B, RANGE_C };

struct range {
    int flags;
    int start;
    int end;
};

struct info {
    struct range ranges[3];
};

static struct info the_info;

void fill(int flags, int start) {
    the_info = (struct info){
        .ranges = {
            [RANGE_A] = {
                .flags = flags,
                .start = start,
                .end = 100,
            },
            [RANGE_B] = {
                .flags = flags + 1,
                .start = start + 1,
                .end = 200,
            },
        },
    };
}

int main(void) {
    fill(1, 2);
    if (the_info.ranges[RANGE_A].flags != 1) return 1;
    if (the_info.ranges[RANGE_A].start != 2) return 2;
    if (the_info.ranges[RANGE_A].end != 100) return 3;
    if (the_info.ranges[RANGE_B].flags != 2) return 4;
    if (the_info.ranges[RANGE_B].start != 3) return 5;
    if (the_info.ranges[RANGE_B].end != 200) return 6;
    /* RANGE_C was never designated: must be zero-initialized. */
    if (the_info.ranges[RANGE_C].flags != 0) return 7;
    if (the_info.ranges[RANGE_C].start != 0) return 8;
    if (the_info.ranges[RANGE_C].end != 0) return 9;
    return 0;
}
