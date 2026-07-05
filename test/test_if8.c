// Test: __GNUC_PREREQ in #if expressions (from glibc features.h)
// This is a real-world test: the >= comparison inside a function-like
// macro must be evaluated correctly in #if expressions.
#define __GNUC_PREREQ(maj, min) ((15<<16)+2 >= ((maj)<<16)+(min))

#if __GNUC_PREREQ(4,1)
// OK — (15<<16)+2 = 983042, (4<<16)+1 = 262145, 983042 >= 262145 = true
#else
#error "FAIL: __GNUC_PREREQ(4,1) should be true"
#endif

#if __GNUC_PREREQ(99,0)
#error "FAIL: __GNUC_PREREQ(99,0) should be false"
#endif

#define MAJOR 15
#define MINOR 0
#if __GNUC_PREREQ(MAJOR, MINOR)
// OK — (15<<16)+2 = 983042, (15<<16)+0 = 983040, 983042 >= 983040 = true
#else
#error "FAIL: __GNUC_PREREQ(15,0) should be true"
#endif

int main(void) { return 0; }
