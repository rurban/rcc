/* "-L path" / "-l name" as two separate argv elements (as opposed to the
 * glued "-Lpath"/"-lname" form) is real GCC/ld syntax that build systems
 * commonly emit from a Make variable, e.g. kefir's own
 * "$(CCLD) ... -L $(LIB_DIR) -lkefir $(LDFLAGS)". A bare "-L"/"-l" alone
 * only matched the leading "-L"/"-l" prefix check and was forwarded to
 * the linker with nothing after it; the actual path/library name in the
 * next argv slot was silently treated as an unrelated positional input
 * file instead of being joined onto the flag -- so "-L $(LIB_DIR) -lfoo"
 * linked with no search path and dropped the library the caller actually
 * asked for, then reported unrelated "-lfoo: undefined reference"
 * errors. Found via kefir's own final "kefir-cc1" link step.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char src[600], bin[620], libdir[620], libsrc[640], cmd[3000];
    int pid = (int)getpid();

    snprintf(libdir, sizeof(libdir), "%s/test_barel_libdir_%d", td, pid);
    if (test_mkdir(libdir) != 0 && errno != EEXIST) { printf("FAIL: mkdir %s\n", libdir); return 1; }

    /* A tiny static library the linker can only find via -L, never on the
     * default search path. */
    snprintf(libsrc, sizeof(libsrc), "%s/answer.c", libdir);
    FILE *lf = fopen(libsrc, "w");
    if (!lf) { printf("FAIL: cannot write %s\n", libsrc); return 2; }
    fputs("int the_answer(void) { return 42; }\n", lf);
    fclose(lf);

    char libobj[660], libar[660];
    snprintf(libobj, sizeof(libobj), "%s/answer.o", libdir);
    snprintf(libar, sizeof(libar), "%s/libanswer.a", libdir);
    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s", rcc, libobj, libsrc);
    if (system(cmd) != 0) { printf("FAIL: building answer.o failed\n"); return 3; }
    snprintf(cmd, sizeof(cmd), "ar rcs %s %s", libar, libobj);
    if (system(cmd) != 0) { printf("FAIL: ar failed (is 'ar' installed?)\n"); return 4; }

    snprintf(src, sizeof(src), "%s/test_barel_main_%d.c", td, pid);
    snprintf(bin, sizeof(bin), "%s/test_barel_main_%d", td, pid);
    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: cannot write %s\n", src); return 5; }
    fputs("extern int the_answer(void);\n"
          "int main(void) { return the_answer() == 42 ? 0 : 1; }\n", sf);
    fclose(sf);

    /* The actual bug: "-L" and the directory as two separate argv
     * elements, "-l" and the library name as two separate argv elements
     * -- exactly what a Make variable like "-L $(LIB_DIR) -lfoo" expands
     * to when passed through a shell command line. */
    snprintf(cmd, sizeof(cmd), "%s -o %s %s -L %s -l answer", rcc, bin, src, libdir);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: link with separated \"-L dir\"/\"-l name\" failed (rc=%d)\n", rc);
        remove(src); remove(libobj); remove(libar); remove(libsrc);
        return 6;
    }
    rc = system(bin);
    remove(src); remove(bin); remove(libobj); remove(libar); remove(libsrc);
    remove(libdir);
    if (rc != 0) {
        printf("FAIL: linked binary did not resolve the_answer() correctly (rc=%d)\n", rc);
        return 7;
    }

    printf("OK\n");
    return 0;
}
