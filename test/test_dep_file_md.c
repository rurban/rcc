/* The bare GCC/Clang dependency flags -MD/-MMD/-MF/-MT (used directly by
 * CMake and ninja, unlike autotools' -Wp,-MMD,) were not recognised: they
 * fell through to the "ignored unknown option" path, so no .d file was
 * written and the build failed with "cannot open <obj>.o.d". Verify rcc
 * now writes the dependency file, uses the -MT target for the rule, and
 * lists the source as a prerequisite. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char src[600], obj[600], dep[600], cmd[2600];
    int pid = (int)getpid();

    snprintf(src, sizeof(src), "%s/test_depmd_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_depmd_%d.o", td, pid);
    snprintf(dep, sizeof(dep), "%s/test_depmd_%d.o.d", td, pid);

    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: cannot write %s\n", src); return 1; }
    fputs("#include <stddef.h>\nint main(void){return 0;}\n", sf);
    fclose(sf);

    /* CMake-style invocation: -MD -MT <target> -MF <file>. */
    snprintf(cmd, sizeof(cmd),
             "%s -MD -MT the_target.o -MF %s -c -o %s %s",
             rcc, dep, obj, src);
    int rc = system(cmd);
    remove(src);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: -MD/-MT/-MF compile failed (rc=%d)\n", rc);
        remove(dep);
        return 2;
    }

    FILE *f = fopen(dep, "r");
    if (!f) { printf("FAIL: no dependency file generated\n"); return 3; }
    char content[8192];
    size_t n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);
    remove(dep);

    /* Rule target must be the -MT value, not the -o object. */
    if (strncmp(content, "the_target.o:", 13) != 0) {
        printf("FAIL: rule target is not the -MT value: %.40s\n", content);
        return 4;
    }
    /* The source file must appear as a prerequisite. */
    if (!strstr(content, src)) {
        printf("FAIL: source not listed as prerequisite: %s\n", content);
        return 5;
    }
    /* A system header pulled in by <stddef.h> should be listed too. */
    if (!strstr(content, ".h")) {
        printf("FAIL: no header prerequisites listed: %s\n", content);
        return 6;
    }
    printf("OK\n");
    return 0;
}
