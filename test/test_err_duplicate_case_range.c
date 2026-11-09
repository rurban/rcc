/* A GNU case range that overlaps another label is a duplicate case value. */
int main(void) {
    switch (5) {
    case 1 ... 10:
        break;
    case 8:
        break;
    }
    return 0;
}
