int printf(const char *, ...);

struct point {
    int x;
    int y;
};

// C11 6.7.2.1p2: a bare semicolon is a valid (empty) member declaration.
// Kernel macros with an empty __VA_ARGS__ expand to exactly this shape.
struct empty_decl {
    int a;
    ;
    int b;
};

int main() {
    struct point p = {10, 20};
    printf("p.x=%d p.y=%d\n", p.x, p.y);

    struct point p2 = {1, 2};
    printf("p2.x=%d p2.y=%d\n", p2.x, p2.y);

    struct empty_decl e = {1, 2};
    if (e.a != 1 || e.b != 2) return 1;

    return 0;
}
