/* GCC extension: the ';' after a struct/union's last member declarator
 * list may be omitted when '}' unambiguously terminates it. GCC accepts
 * this with "warning: no semicolon at end of struct or union"; rcc used
 * to hard-error via skip(tok, ";") instead of tolerating it (surfaced by
 * GCC PR101290's deeply nested anonymous struct/union reproducer).
 */
extern int printf(const char *, ...);

struct s {
    int a;
    int b
};

union u {
    int a;
    long b
};

int main(void)
{
    struct s v = { 1, 2 };
    union u w;
    w.a = 3;
    printf("%d %d %d\n", v.a, v.b, w.a);
    return (v.a == 1 && v.b == 2 && w.a == 3) ? 0 : 1;
}
