/* The GCC include-path flags -iquote / -isystem / -idirafter each take a
 * directory argument. rcc did not recognise them, so the flag was warned
 * about and IGNORED -- and its directory argument was then left to be
 * misparsed as an input file ("error: <dir>: file too large", e.g.
 * noplate's `-iquote ./src`). Verify the argument is consumed AND the
 * directory is actually added to the include search path. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char dir[600], hdr[700], src[600], obj[600], cmd[3200];
    int pid = (int)getpid();

    snprintf(dir, sizeof(dir), "%s/test_iquote_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    snprintf(hdr, sizeof(hdr), "%s/iq_header.h", dir);
    snprintf(src, sizeof(src), "%s/test_iquote_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_iquote_%d.o", td, pid);

    FILE *hf = fopen(hdr, "w");
    if (!hf) { printf("FAIL: write header\n"); return 2; }
    fputs("#define IQ_OK 1\n", hf);
    fclose(hf);

    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: write src\n"); return 3; }
    /* Header is reachable ONLY through the -iquote directory. */
    fputs("#include \"iq_header.h\"\n"
          "#if !IQ_OK\n#error not_found\n#endif\n"
          "int main(void){return 0;}\n", sf);
    fclose(sf);

    const char *flags[] = {"-iquote", "-isystem", "-idirafter"};
    int rv = 0;
    for (int i = 0; i < 3; i++) {
        snprintf(cmd, sizeof(cmd), "%s %s %s -c -o %s %s",
                 rcc, flags[i], dir, obj, src);
        int rc = system(cmd);
        remove(obj);
        if (rc != 0) {
            printf("FAIL: %s did not add include dir / consumed arg (rc=%d)\n",
                   flags[i], rc);
            rv = 4;
            break;
        }
    }

    remove(hdr);
    remove(src);
    rmdir(dir);
    if (rv == 0) printf("OK\n");
    return rv;
}
