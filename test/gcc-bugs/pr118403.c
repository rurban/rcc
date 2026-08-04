/* GCC Bug #118403 - uninitialized warning with automatic union
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118403
 */


// A union has just one member active at a time.  If you don't use designated initializer, then {0} just initializes the first member in there.  All the remaining bits are padding.
struct A { char a; /* 7 padding bytes */ long long b; };
struct B { int c; /* 4 padding bytes */ struct A d; };
void foo ()
{
//   struct B e = { 0 }; // This doesn't guarantee initialization of the 4 padding bytes,
//                       // but does guarantee initialization of the 7 padding bytes
//   struct B f = { 0, { 0 } }; // This one doesn't guarantee initialization of any padding bytes, just the fields.
//   struct B g = { 0, { 0, 0 } }; // Neither this.
}
// Now, C23 adds {} initializers and those are also guaranteed to zero padding bits, so
// ...
  struct B h = {}; // Guarantees zero initialization of all padding bits
//   struct B i = { 0, {} }; // Guarantees initialization of the 7 padding bytes but not the 4.
// If you use struct nic_mbx mbx = { }; then all bits including padding are zero initialized, { .msg = { 0 } }; means that .msg field has its first member initialized but doesn't say anything about padding bits.
// You could say also { .whatever_field_is_largest = { 0 } } and initialize more bits.


