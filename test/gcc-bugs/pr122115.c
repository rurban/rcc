/* GCC Bug #122115 - Add warning to catch implicit type conversions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122115
 */


// Not real code, just example
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// size_t f()
{
    return (size_t)rand();
}

// size_t f_arg(size_t val)
{
    return (size_t)rand() * val;
}
// enum
{
//     myenum,
//     otherenum = -1
};

int main()
{
//     // Implicit conversion of return
    bool foo = f();
    __builtin_printf("%d\n", foo);

//     // implicit conversion: int -> bool (nonzero becomes true)
    int n = 5;
    bool b = n;

    __builtin_printf("n=%d, b=%d\n", n, b);

//     // Implicit conversion of argument
    size_t ret = f_arg(foo);
    __builtin_printf("%zu\n", ret);

//     // Implicit conversion in arithmetic
    size_t sum = b*foo;
    __builtin_printf("sum %zu\n", sum);

//     // Implicit conversion of enum
    size_t enum_copy = myenum;
    __builtin_printf("enum_copy %zu\n", enum_copy);

//     // Implicit conversion of *negative* enum to bool
    bool enum_bool = otherenum;
    __builtin_printf("enum_bool %d\n", enum_bool);

//     // Implicit conversion in conditions
    if(f()) __builtin_printf("f() true\n");
}
// Warnings such as:
// I'm mostly interested in the implicit conversion to bool in return (assignment to size_t) in this example.
// Yes, I appreciate the C language allows all these implicit conversions, and they convert to int. clang-tidy has some checkers for these sort of things.
// I recall C99 added _Bool so it should be a real type.
// I couldn't get warnings from this program using -Wconversion and -std=c99
// <a href="https://godbolt.org/z/Wo6qGEMT3">https://godbolt.org/z/Wo6qGEMT3</a>


