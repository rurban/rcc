/* GCC Bug #89107 - -Wconversion warning is not appropriate since conversion doesn't alter value, because of mask entered before.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89107
 */


union U {
    unsigned int a:20;
};

int main() {
    union U u;
    unsigned int val = 0xaabbc000;

//     u.a = val & 0xfffff;         // 1) works
//     u.a = (val >> 12) & 0xfffff; // 2) doesn't
}
//      u.a = (val >> 12) & 0xfffff; /* 2) doesn't */

// Clang is compiling following code successfully. Seems like there is some off-by-one bug; Mask 0x7ffff works without warnings/errors.


