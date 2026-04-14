int main() {
    _foo = 100;
    _bad_ptr = *_foo; // Here is a semantic type error! _foo is int!
    return 1;
}
