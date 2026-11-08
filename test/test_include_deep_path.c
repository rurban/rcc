/* canonical_path() split an include path into at most 256 components in
 * fixed arrays, so `#include "/tmp/x/d/d/d/.../h.h"` with more than 256
 * directory levels wrote past them and rcc crashed. The arrays now cover the
 * longest possible 4096-byte path.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

#ifdef _WIN32
int main(void) {
    printf("OK (skipped)\n");
    return 0;
}
#else
#include <sys/stat.h>
#include <unistd.h>

#define LEVELS 300

int main(void) {
    const char *rcc = find_rcc();
    char base[160], dir[2048], hdr[2200], src[200], obj[200], cmd[1200];
    snprintf(base, sizeof(base), "%s/test_deep_%d", get_tmpdir(), (int)getpid());
    snprintf(dir, sizeof(dir), "%s", base);
    if (mkdir(base, 0700) != 0) {
        printf("FAIL: mkdir %s\n", base);
        return 1;
    }
    int made = 0;
    for (; made < LEVELS; made++) {
        strcat(dir, "/d");
        if (mkdir(dir, 0700) != 0) break;
    }
    snprintf(hdr, sizeof(hdr), "%s/h.h", dir);
    FILE *f = made == LEVELS ? fopen(hdr, "w") : NULL;
    int rc = 1;
    if (f) {
        fputs("int deep_val = 42;\n", f);
        fclose(f);
        snprintf(src, sizeof(src), "%s.c", base);
        snprintf(obj, sizeof(obj), "%s.o", base);
        f = fopen(src, "w");
        if (f) {
            fprintf(f, "#include \"%s\"\nint f(void) { return deep_val; }\n", hdr);
            fclose(f);
            snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
            rc = system(cmd);
        }
        remove(src);
        remove(obj);
        remove(hdr);
    }
    /* unwind the directory chain */
    for (int i = made; i > 0; i--) {
        char *slash = strrchr(dir, '/');
        if (!slash) break;
        rmdir(dir);
        *slash = '\0';
    }
    rmdir(base);
    if (made != LEVELS) {
        printf("FAIL: could only create %d levels\n", made);
        return 1;
    }
    if (rc != 0) {
        printf("FAIL: rcc failed on a %d-level include path\n", LEVELS);
        return 1;
    }
    printf("OK\n");
    return 0;
}
#endif
