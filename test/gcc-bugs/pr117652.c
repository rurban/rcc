/* GCC Bug #117652 - [14/15/16/17 regression] ICE: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in tagged_types_tu_compatible_p, at c/c-typeck.cc:1919
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117652
 */
/* { dg-do compile } */


struct foo {
  int counter;
  struct bar array[] __attribute__((counted_by(counter)));
} *p;

#define alloc(P, FAM, COUNT) ({ \
  size_t __size = sizeof(*P) + sizeof(*P->FAM) * COUNT; \
//   kmalloc(__size, GFP); \
})

p = alloc(p, array, how_many);
// p->counter = how_many;

#define alloc(P, FAM, COUNT) ({ \
  typeof(P) __p; \
  size_t __size = sizeof(*P) + sizeof(*P->FAM) * COUNT; \
  __p = kmalloc(__size, GFP); \
  __builtin_set_counted_by(__p->FAM, COUNT); \
//   __p; \
})

void __builtin_set_counted_by (ptr->FAM, const_exp_with_int_type)

void __builtin_set_counted_by (FAM_exp, count_exp)

void __builtin_set_counted_by (void *, size_t)

void __builtin_set_counted_by (ptr, type expr)

struct foo1 {
  int counter1;
  struct bar1 array[] __attribute__((counted_by(counter)));
} *p;

struct foo2 {
  int other;
  struct bar2 array[];
} *q;

__builtin_set_counted_by (p->array, COUNT)

__builtin_set_counted_by (q->array, COUNT)

struct foo1 {
  int counter1;
  struct bar1 array[] __attribute__((counted_by(counter)));
} *p;

struct foo2 {
  int other;
  struct bar2 array[];
} *q;

__builtin_set_counted_by (p->array, COUNT)

__builtin_set_counted_by (q->array, COUNT)

void __builtin_set_counted_by (ptr, type expr)

void __builtin_set_counted_by (ptr, type expr)

struct foo {
  int counter;
  struct bar1 array[] __attribute__((counted_by(counter)));
} *p;

__builtin_set_counted_by (p->array, COUNT)

struct foo2 {
  int other;
  struct bar2 array[];
} *q;

__builtin_set_counted_by (q->array, COUNT)

#define alloc(P, FAM, COUNT) ({ \
  typeof(P) __p; \
  size_t __size = sizeof(*P) + sizeof(*P->FAM) * COUNT; \
  __p = kmalloc(__size, GFP); \
  if (__p && __builtin_get_counted_by(__p->FAM)) \
    *__builtin_get_counted_by(__p->FAM) = COUNT; \
//   __p; \
})

struct foo {
  int counter;
  struct bar array[] __attribute__((counted_by(counter)));
} *p;

__builtin_set_counted_by (p->array, COUNT)
//    13 | p = alloc(p, array, how_many);
//     6 | } *p;
//     8 | #define alloc(P, FAM, COUNT) ({ \
//    13 | p = alloc(p, array, how_many);
//    14 | p->counter = how_many;
//    16 | #define alloc(P, FAM, COUNT) ({ \
//     8 | #define alloc(P, FAM, COUNT) ({ \
//    24 | void __builtin_set_counted_by (ptr->FAM, const_exp_with_int_type)
//       |                                   )
//    35 | } *p;
//    35 | } *p;
//    39 |   struct bar2 array[];
//    42 | __builtin_set_counted_by (p->array, COUNT)
//       |                            )
//    49 | } *p;
//    49 | } *p;
//    53 |   struct bar2 array[];
//    54 | } *q;
// 0x259f775 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x25b6165 internal_error(char const*, ...)
// 0x8beab5 tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0xa03af2 comptypes_same_p(tree_node*, tree_node*)
// 0x9f3f21 finish_struct(unsigned int, tree_node*, tree_node*, tree_node*, c_struct_parse_info*, tree_node**)
// 0xa563b4 c_parser_declspecs(c_parser*, c_declspecs*, bool, bool, bool, bool, bool, bool, bool, c_lookahead_kind)
// 0xa71e2b c_parse_file()
// 0xaee499 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


