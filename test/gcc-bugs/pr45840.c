/* GCC Bug #45840 - Enhance __builtin_object_size to return useful result when applied to T (*p)[N]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45840
 */


// Created <span class=""><a href="attachment.cgi?id=21919" name="attach_21919" title="Preprocessed source which demonstrates the missed opportunity for __builtin_object_size">attachment 21919</a> <a href="attachment.cgi?id=21919&action=edit" title="Preprocessed source which demonstrates the missed opportunity for __builtin_object_size">[details]</a></span>
// Preprocessed source which demonstrates the missed opportunity for __builtin_object_size
// In the attached test case, there are four functions that use __builtin_object_size.  Two of them always report the size of the underlying object, but the other two report it only when inlining allows gcc to fold the function into the caller.  The functions which always report the correct value (al, sl) use __builtin_object_size on a local variable.  The functions that only work with inlining (ap, sp) specify via their prototype that they require a pointer to an object of a specific size.  The documentation says that __builtin_object_size only works if the compiler knows all possible objects to which the pointer can refer, which is presumably why it presently returns no answer for the pointer cases.  However, any caller which deviates from the size demanded by the prototype would either get a type mismatch warning or be using a cast to suppress such a warning.  Therefore, it seems like the compiler would be within its rights to claim the object is the size that the type info says it should be, since any cases where it is wrong occur because someone violated the function prototype.
// Making this change would allow source fortification macros to be effective on constructs that are presently not checked due to __builtin_object_size returning that it cannot determine the size.  I can attach a sample file showing this as well, but that is much more complex due to the use of the static inline fortification helpers.
// Hopefully, the gcc code can distinguish this case from the more common case of "void f(char *s)", where the size of the object pointed at by s is not known and it is correct for __builtin_object_size to return no answer.

// Technically, the test case could be reduced by considering either the set of (al, ap) or the set of (sl, sp).  I included both to show that __builtin_object_size is cautious both when dealing with a pointer to an array and when dealing with a pointer to a structure.
// Details requested by "Reporting Bugs" page:
// gcc version: gcc version 4.5.1 (Gentoo Hardened 4.5.1 p1.0, pie-0.4.5)
// System type: Gentoo/Linux 2.6.35.6 x86_64-pc-linux-gnu
// Options used for building gcc:
//     Configured with: /var/tmp/portage/portage/sys-devel/gcc-4.5.1/work/gcc-4.5.1/configure --prefix=/usr --bindir=/usr/x86_64-pc-linux-gnu/gcc-bin/4.5.1 --includedir=/usr/lib/gcc/x86_64-pc-linux-gnu/4.5.1/include --datadir=/usr/share/gcc-data/x86_64-pc-linux-gnu/4.5.1 --mandir=/usr/share/gcc-data/x86_64-pc-linux-gnu/4.5.1/man --infodir=/usr/share/gcc-data/x86_64-pc-linux-gnu/4.5.1/info --with-gxx-include-dir=/usr/lib/gcc/x86_64-pc-linux-gnu/4.5.1/include/g++-v4 --host=x86_64-pc-linux-gnu --build=x86_64-pc-linux-gnu --disable-altivec --disable-fixed-point --without-ppl --without-cloog --disable-lto --disable-nls --with-system-zlib --disable-werror --enable-secureplt --enable-multilib --enable-libmudflap --disable-libssp --enable-esp --enable-libgomp --enable-cld --with-python-dir=/share/gcc-data/x86_64-pc-linux-gnu/4.5.1/python --enable-checking=release --disable-libgcj --enable-languages=c,c++ --enable-shared --enable-threads=posix --enable-__cxa_atexit --enable-clocale=gnu --with-bugurl=<a href="http://bugs.gentoo.org/">http://bugs.gentoo.org/</a> --with-pkgversion='Gentoo Hardened 4.5.1 p1.0, pie-0.4.5'
// Complete command line:
//     With inlining (works because object becomes local): gcc-4.5.1 -O2 bos.c -o bos-O2
//     Without inlining (fails because gcc decides it cannot prove it knows all possible pointer targets): gcc-4.5.1 -O2 -fno-inline bos.c -o bos-O2-no-inline
// Compiler output: none, since the code triggers no warnings and demonstrates a missed opportunity, not a compiler bug.
// Preprocessed output: attached, to preserve whitespace.
// I also tried the test program on Gentoo gcc-4.3.4 and Gentoo gcc-4.4.4.  They behaved the same way as the gcc-4.5.1 output shown.
// Actual output:
// $ ./bos-O2
// ap:11: s=20 b0=20 b1=20 b2=20 b3=20
// sp:16: s=20 b0=20 b1=20 b2=20 b3=20
// al:23: s=20 b0=20 b1=20 b2=20 b3=20
// al:24: s=20 b0=20 b1=20 b2=20 b3=20
// sl:30: s=20 b0=20 b1=20 b2=20 b3=20
// $ ./bos-O2-no-inline
// ap:11: s=20 b0=-1 b1=-1 b2=0 b3=0
// sp:16: s=20 b0=-1 b1=-1 b2=0 b3=0
// al:23: s=20 b0=20 b1=20 b2=20 b3=20
// al:24: s=20 b0=20 b1=20 b2=20 b3=20
// sl:30: s=20 b0=20 b1=20 b2=20 b3=20
// Desired output:
// bos-O2-no-inline would report a known size on the basis of the prototype, even if not all targets were definitely known.


