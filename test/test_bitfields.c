int printf(const char*, ...);

struct bf { unsigned x:3; unsigned y:5; unsigned z:8; };

int main() {
    struct bf b = {1, 2, 3};
    printf("x=%d y=%d z=%d\n", b.x, b.y, b.z);

    b.x = 7;
    b.y = 15;
    b.z = 255;
    printf("x=%d y=%d z=%d\n", b.x, b.y, b.z);

    // Test compound assignment
    b.x += 1;
    printf("x+=1: %d\n", b.x);

    // Test bitfield with different types
    struct bf2 { char a:4; short b:8; int c:16; } b2 = {1, 2, 3};
    printf("b2: a=%d b=%d c=%d\n", b2.a, b2.b, b2.c);

    return 0;
}