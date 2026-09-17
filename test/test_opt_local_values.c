/* -O1 block-local value propagation and dead stores. */
static int hits;

static int touch(void) {
    return ++hits;
}

static int const_chain(void) {
    int a = 4;
    int b = a + 2;
    int c = b + 3;
    return c;
}

static int copies_survive_source_write(int seed) {
    int a = seed;
    int b = a;
    a = 99;
    return b * 100 + a;
}

static int side_effect_store(void) {
    int a = touch();
    a = 7;
    return hits + a;
}

static int address_taken(void) {
    int a = 1;
    int *p = &a;
    a = 2;
    return *p;
}

int main(void) {
    if (const_chain() != 9) return 1;
    if (copies_survive_source_write(3) != 399) return 2;
    if (copies_survive_source_write(-2) != -101) return 3;
    if (side_effect_store() != 8 || hits != 1) return 4;
    if (address_taken() != 2) return 5;
    return 0;
}
