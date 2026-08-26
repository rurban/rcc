/* A function-local `static` variable's array-element address (or a
 * `.member`/`[N]` chain reached through it) used to initialize ANOTHER
 * static/global object's pointer field silently under-consumed tokens
 * and desynced the parser -- "expected specific operator" (or similar
 * cascading nonsense errors) several tokens later, even though the
 * actual C is completely valid and GCC accepts it without complaint.
 *
 * Root cause: `read_global_label_initializer()` resolves a bare
 * identifier to its storage label (a local static's mangled
 * `.Lstatic.N` asm label for a block-scope `static`), then walks a
 * trailing `[N][M].member...` chain by looking that label back up via
 * `find_global_name()` -- a PURE HASH-TABLE lookup with no linked-list
 * fallback. Every OTHER path that creates a global-storage LVar (the
 * ordinary `new_var()` helper) registers it in that hash table via
 * `global_htab_add()`, but the block-scope-`static` declaration path
 * built its global-storage LVar by hand (`arena_alloc` + manually
 * linking into the `globals` list) and never called `global_htab_add()`
 * at all. So the label lookup always missed, the `[N][M].member...`
 * chain-walk loop's `while (cur_ty)` never even started (`cur_ty` was
 * NULL), and the caller returned with the token cursor sitting right
 * before the still-unconsumed "[0]" -- corrupting every subsequent
 * initializer statement/token relative to that.
 *
 * Found via postgres's `src/interfaces/ecpg/preproc/descriptor.c`:
 *   static char descriptor_names[2][MAX_DESCRIPTOR_NAMELEN];
 *   static struct variable varspace[2] = {
 *       {descriptor_names[0], &descriptor_type, 0, NULL},
 *       {descriptor_names[1], &descriptor_type, 0, NULL}
 *   };
 * Fixed by calling global_htab_add() when creating a block-scope
 * static's global-storage entry, exactly like every other global. */
#include <string.h>

struct variable {
    char *name;
    int x;
};

static struct variable *test(int input) {
    static char names[2][8];
    static struct variable vars[2] = {
        {names[0], 10},
        {names[1], 20},
    };
    strcpy(names[input], "hi");
    return &vars[input];
}

/* Plain (no struct wrapping) local-static pointer initialized from
 * another local-static array's element address -- the minimal shape
 * that triggered the desync. */
static char *plain_ptr_case(void) {
    static char buf[8][8];
    static char *p = buf[0];
    strcpy(buf[0], "ok");
    return p;
}

int main(void) {
    struct variable *v0 = test(0);
    struct variable *v1 = test(1);
    int ok = 1;
    ok = ok && v0 && strcmp(v0->name, "hi") == 0 && v0->x == 10;
    ok = ok && v1 && strcmp(v1->name, "hi") == 0 && v1->x == 20;
    ok = ok && strcmp(plain_ptr_case(), "ok") == 0;
    return ok ? 0 : 1;
}
