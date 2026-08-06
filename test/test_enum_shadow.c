// Regression: an identifier declared in an inner scope must shadow an
// outer-scope enum constant of the same name (C11 6.2.1p4).  rcc's
// primary() used to consult find_enum_const() before find_var(), so a
// local variable named like a global enumerator (e.g. `directory`
// shadowing `enum filetype { ..., directory, ... }`) resolved to the
// enum's numeric value instead of the variable, turning
// `directory = (DIR *)NULL;` into an assignment to a non-lvalue and
// crashing codegen with "Invalid register -1".  Seen compiling
// readline's complete.c (lib/readline/colors.h declares that enum;
// rl_filename_completion_function() declares `static DIR *directory;`).
#include <assert.h>
#include <dirent.h>
#include <stdint.h>

enum filetype { unknown, fifo, chardev, directory, blockdev, normal, symbolic_link };

// Enum constant usable when no variable of that name is in scope.
static int enum_const_unshadowed(void)
{
    return directory; /* == 3 */
}

// Local variable shadows the global enumerator of the same name.
static int local_shadows_enum(void)
{
    int directory = 42;
    directory = directory + 1;
    return directory; /* 43 */
}

// The exact readline shape: static local `directory` used as an lvalue
// in the same function that the enum constant is globally visible in.
static char *static_local_shadows_enum(int state)
{
    static DIR *directory = (DIR *)0;
    if (state == 0) {
        if (directory) {
            closedir(directory);
            directory = (DIR *)0;
        }
    }
    return (char *)0;
}

int main(void)
{
    assert(enum_const_unshadowed() == 3);
    assert(local_shadows_enum() == 43);
    assert(static_local_shadows_enum(0) == (char *)0);
    assert(static_local_shadows_enum(1) == (char *)0);
    return 0;
}
