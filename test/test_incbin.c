/* `.incbin "path"` — embed another file's raw bytes verbatim into the
 * current section — wasn't implemented at all: handle_directive() had no
 * case for it, so it silently fell through to the catch-all "ignored"
 * tail, contributing zero bytes to the section no matter how large the
 * target file actually was.
 *
 * Found via a real Linux kernel build: usr/initramfs_data.S wraps the
 * kernel's built initramfs cpio archive with exactly this directive,
 * between __irf_start/__irf_end labels used elsewhere to compute the
 * blob's size — with .incbin silently emitting nothing, that computed
 * size (a real label-difference, see test_standalone_asm_file.c for that
 * separate bug) came out 0 regardless, masking two independent bugs
 * behind the same symptom.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char datf[128], srcf[128], objf[128], cmd[512];
    snprintf(datf, sizeof(datf), "%s/test_incbin_data_%d.bin", td, pid);
    snprintf(srcf, sizeof(srcf), "%s/test_incbin_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_incbin_%d.o", td, pid);

    /* A small, distinctive binary payload (not valid ASCII/UTF-8, to
     * catch a text-mode/binary-mode file-reading mistake). */
    static const unsigned char payload[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0xFF};
    FILE *df = fopen(datf, "wb");
    if (!df) { printf("FAIL: cannot write %s\n", datf); return 1; }
    fwrite(payload, 1, sizeof(payload), df);
    fclose(df);

    char src[512];
    snprintf(src, sizeof(src),
             ".section .mydata,\"a\"\n"
             "blob_start:\n"
             ".incbin \"%s\"\n"
             "blob_end:\n",
             datf);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); remove(datf); return 2; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    remove(datf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 3;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .mydata %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 4; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* objdump -s prints hex bytes as "deadbeef 000102ff" (space every 4
     * bytes); collapse whitespace so a match doesn't depend on exact
     * column alignment. */
    char collapsed[512];
    size_t ci = 0;
    for (size_t i = 0; out[i] && ci + 1 < sizeof(collapsed); i++)
        if (!isspace((unsigned char)out[i])) collapsed[ci++] = out[i];
    collapsed[ci] = '\0';

    if (!strstr(collapsed, "deadbeef000102ff")) {
        printf("FAIL: expected .incbin's exact bytes deadbeef000102ff in "
               ".mydata, got:\n%s\n", out);
        return 1;
    }

    printf("OK .incbin embeds a raw binary file's bytes verbatim\n");
    return 0;
}
