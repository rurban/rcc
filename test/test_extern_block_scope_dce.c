// Regression test: a block-scope `extern` declaration for a global that is
// *later* defined at file scope, when it appears inside a `static inline`
// function that itself is never called (so opt.c's
// eliminate_unused_static_inline() drops the function as dead code), must
// not drop the referenced global along with it.
//
// Root cause: new_var() (parser.c) unconditionally stamps
// `decl_fn_name = parser_current_fn` on every non-local LVar created while
// inside a function body. That tagging exists so a true block-scope
// `static` (which owns its own storage, tied to the enclosing function's
// lifetime - see the `attr.is_static` branch) gets dropped alongside its
// function when that function is DCE'd. But the block-scope `extern`
// branch reused the same new_var() path, so the *global* it merely
// references (owned independently at file scope, not by the function)
// got the same "drop with the function" tag. When the file-scope
// definition below reused this exact LVar (find_global_name() found it
// already registered) and `helper()` was never called, opt.c's second
// DCE pass spliced `preserve_acls` out of prog->globals entirely -
// producing an object with only an unresolved `U preserve_acls` reference
// and no definition at all, i.e. a link failure ("undefined reference to
// `preserve_acls'") in any program that also calls `use_it()` below.
//
// Found via rsync's options.c: `extern int preserve_acls;` inside an
// unused `static inline` accessor, with the real `int preserve_acls = 0;`
// definition elsewhere in the same TU - rsync failed to link with
// "undefined reference to `preserve_acls'"/`preserve_xattrs'` until this
// fix (test/third_party/TODO.md, test_rsync).
#include <assert.h>

static inline void unused_accessor(void) {
    extern int preserve_acls;
    if (preserve_acls)
        preserve_acls = 2;
}

int preserve_acls = 0;

// Second field-alike case: the extern appears in a *different* unused
// static inline function than the one that references the value later,
// exercising decl_fn_name tagging from more than one dead function.
static inline void another_unused_accessor(void) {
    extern int preserve_xattrs;
    if (preserve_xattrs)
        preserve_xattrs = 2;
}

int preserve_xattrs = 7;

static int use_it(void) {
    preserve_acls = 1;
    return preserve_acls + preserve_xattrs;
}

int main(void) {
    assert(use_it() == 1 + 7);
    assert(preserve_acls == 1);
    assert(preserve_xattrs == 7);
    return 0;
}
