void abort(void);

static int call_ptr(int (*fn)(int *), int *p) {
    return fn(p);
}

static int outer(void) {
    int i = 0;

    int inner(int *p) {
        i = 1;
        return *p + 1;
    }

    return call_ptr(inner, &i);
}

int main(void) {
    if (outer() != 2)
        abort();
    return 0;
}
