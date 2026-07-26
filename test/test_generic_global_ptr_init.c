/* `_Generic` is tokenized as a plain identifier (no dedicated token kind) —
 * fine for its use inside a function body, but a *file-scope* pointer
 * variable's initializer goes through a separate fast path first
 * (read_global_label_initializer()) that recognizes a handful of simple
 * address-constant shapes ("&label", a string literal, "&&label") before
 * falling back to full expression parsing. That fast path's plain-
 * identifier branch didn't know "_Generic" was a keyword starting a whole
 * selection expression, not a bare symbol name: it treated the literal
 * text "_Generic" itself as the label to reference and stopped right
 * there, leaving the parser staring at the following "(" and reporting a
 * confusing "expected ';' or ','" several tokens later — nothing in the
 * message pointed at _Generic as the actual cause.
 *
 * Found via a real Linux kernel build: init/version-timestamp.c's
 * init_uts_ns definition (through <linux/ns_common.h>'s NS_COMMON_INIT()
 * macro) selects its .ops field's proc_ns_operations pointer with
 * _Generic((&init_uts_ns), struct cgroup_namespace *: &cgroupns_operations,
 * ..., struct uts_namespace *: &utsns_operations) — a `_Generic` whose
 * selected association is itself a pointer, used directly as a file-scope
 * pointer initializer. A local variable or a non-pointer global with the
 * exact same _Generic shape worked fine already; only this specific
 * combination (file scope + pointer-typed target) went through the buggy
 * fast path.
 */
#include <stddef.h>

struct foo { int x; };
struct bar { int y; };

struct foo the_foo;
struct bar the_bar;

int selector_int;

/* The exact shape that broke: a file-scope pointer variable whose
 * initializer is a bare _Generic() selecting an address-of-global. */
struct foo *foo_ptr = _Generic(selector_int, int: &the_foo);

/* The real-world shape: several struct-pointer associations, one of them
 * matching the controlling expression's own (self-referential) type. */
struct bar *bar_ptr = _Generic((struct bar *)0,
    struct foo *: &the_foo,
    struct bar *: &the_bar);

int main(void)
{
    if (foo_ptr != &the_foo) return 1;
    if (bar_ptr != &the_bar) return 2;
    return 0;
}
