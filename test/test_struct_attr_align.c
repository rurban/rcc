/* A struct/union whose OWN trailing __attribute__((aligned(N))) widens its
 * alignment must also pad its own size up to that alignment — every array
 * element needs the same alignment, so the type's stride (sizeof) has to
 * be a multiple of it, same as GCC. Kernel FOLIO_MATCH()-style layout
 * checks (mm_types.h) rely on the resulting size.
 *
 * A declaration-level _Alignas/aligned() on one object must NOT pad the
 * type's own size — that only raises the alignment requirement of that
 * one declared object (regression check: pr68532/pr63148).
 */

struct page {
    long a;
    int b;
} __attribute__((aligned(2 * sizeof(long))));

static_assert(_Alignof(struct page) == 2 * sizeof(long),
              "trailing aligned() must widen the struct's alignment");
static_assert(sizeof(struct page) % (2 * sizeof(long)) == 0,
              "trailing aligned() must pad the struct's own size to its alignment");

_Alignas(16) unsigned short over_aligned_array[4];
static_assert(sizeof(over_aligned_array) == 4 * sizeof(unsigned short),
              "declaration-level _Alignas must not pad each element's size");

/* GNU extension: `struct TAG;` referencing a previously completed
 * struct/union, with no declarator and no fresh body, is an unnamed
 * member whose fields get promoted into the enclosing struct — same as
 * an untagged `struct { ... };` (e.g. kernel's `struct filename`). */
struct inner_head { int id; };
struct filename_like {
    struct inner_head;
    const char *name;
};

/* A qualifier may precede the struct/union keyword of an anonymous
 * member (`const struct { ... };`, seen in linux/mm.h's vm_fault). */
struct vm_fault_like {
    const struct { int flags; };
    void *addr;
};

/* GNU extension: a *tagged* struct/union with its own fresh body and no
 * declarator is ALSO an anonymous promoted member — the tag just makes
 * it separately nameable elsewhere, e.g. the kernel's socket_lock_t:
 *   union { struct slock_owned { int owned; long slock; }; long combined; };
 * Distinct from the no-fresh-body case above: this one defines a brand
 * new type (registered under its tag) rather than referencing one that
 * already exists. */
typedef struct {
    union {
        struct slock_owned {
            int owned;
            long slock;
        };
        long combined;
    };
    int wq;
} socket_lock_t;

struct slock_owned standalone_slock; /* the tag must be independently usable */

int main(void)
{
    struct filename_like f;
    f.id = 42; /* promoted member from struct inner_head */
    if (f.id != 42) return 1;

    struct vm_fault_like vf;
    vf.flags = 7; /* promoted member from const struct {...} */
    if (vf.flags != 7) return 2;

    socket_lock_t sl;
    sl.owned = 5; /* promoted member from tagged struct slock_owned {...} */
    sl.slock = 9;
    if (sl.owned != 5 || sl.slock != 9) return 3;
    sl.combined = 42; /* sibling anonymous union member still reachable */

    standalone_slock.owned = 1;
    if (standalone_slock.owned != 1) return 4;

    return 0;
}
