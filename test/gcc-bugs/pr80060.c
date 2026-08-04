/* GCC Bug #80060 - RFE: -Wformat knob to ignore same-width incorrect types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80060
 */
/* { dg-do compile } */


struct uffd_msg msg;
struct uffd_msg {
// ..
        union {
                struct {
                        __u64   flags;
                        __u64   address;
                } pagefault;
// ..
        } arg;
}

// Printing one of these values is obvious - the typedef name tells you it should be an unsigned 64-bit quantity, so let's try printf("%"PRIx64, msg.arg.pagefault.flags).  Oops, on 64-bit Linux, that fails under -Wformat, because "%ld" is incompatible with 'unsigned long long'.  But since the kernel headers typedef'd __u64 without any counterpart to something like PRIx64, there is no sane way to print a __u64 without writing an extra cast at every caller, which is prone to introduce more bugs than the warnings it silences: printf("%"PRIx64, (uint64_t)msg.arg.pagefault.flags).

// Another case: on 32-bit mingw, the declaration for ntohl() says that it returns a 'u_long', which is a 32-bit type.  But POSIX says that ntohl() returns 'uint32_t'.  So the obvious printf("%"PRIx32, ntohl(1)) fails to compile on mingw, because 'u_long' (which is 'unsigned long') is incompatible with 'uint32_t' (which is 'unsigned int') on that platform (arguably a bug in mingw's headers, but such is life).  Again, the mismatch warning can be avoided with a cast, but that does not scale well: printf("%"PRIx32, (uint32_t)ntohl(1))
// So I'd love to have a new knob, maybe named -Wformat-same-rank, which controls whether gcc warns about using the wrong format specifier EVEN THOUGH the type passed to varargs has the same rank and therefore will print accurately; and projects can then use -Wno-format-same-rank to silence __u64/uint64_t or u_long/uint32_t differences while still getting warnings about real bugs of 32-bit vs. 64-bit mismatches.


