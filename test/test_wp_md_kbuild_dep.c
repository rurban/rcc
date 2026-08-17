/* Linux kernel/busybox Kbuild's scripts/Makefile.host passes dependency
 * generation as `-Wp,-MD,<file>` (single M) rather than autotools'
 * `-Wp,-MMD,<file>` (double M, already supported) -- rcc only recognized
 * the double-M spelling, so the single-M form fell through to the
 * "ignored unknown option" path: no .d file was ever written, and
 * kbuild's fixdep then failed with "<file>.d: No such file or
 * directory" building scripts/basic/fixdep itself (test/third_party's
 * test_busybox). Fixed by recognizing `-Wp,-MD,<file>` identically to
 * `-Wp,-MMD,<file>` (rcc doesn't distinguish system vs. non-system
 * headers in its .d output either way, so the two forms are equivalent
 * here).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char src[600], obj[600], dep[600], cmd[2048];
    int pid = (int)getpid();

    snprintf(src, sizeof(src), "%s/test_wp_md_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_wp_md_%d.o", td, pid);
    snprintf(dep, sizeof(dep), "%s/test_wp_md_%d.d", td, pid);

    FILE *sf = fopen(src, "w");
    if (!sf) {
        printf("FAIL: cannot write %s\n", src);
        return 1;
    }
    fputs("int main(void){return 0;}\n", sf);
    fclose(sf);

    snprintf(cmd, sizeof(cmd), "%s -Wp,-MD,%s -c -o %s %s", rcc, dep, obj, src);
    int rc = system(cmd);
    remove(src);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: compile with -Wp,-MD, failed (rc=%d)\n", rc);
        return 1;
    }

    FILE *f = fopen(dep, "r");
    if (!f) {
        printf("FAIL: -Wp,-MD, did not generate a dependency file\n");
        return 2;
    }
    char content[4096];
    size_t n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);
    remove(dep);

    /* Must contain the object's own rule target and reference the source. */
    if (!strstr(content, ".o") || !strstr(content, "test_wp_md")) {
        printf("FAIL: dependency file looks wrong: %s\n", content);
        return 3;
    }

    printf("OK\n");
    return 0;
}
