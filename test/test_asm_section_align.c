/* `.align N`/`.balign N`/`.p2align N` on a *built-in* section
 * (`.text`/`.data`/`.rodata`/`.tdata`/...) correctly padded the
 * requesting object file's own buffer so a label right after it landed
 * on the requested boundary *relative to that file's own section
 * start* -- but never raised the section's own ELF sh_addralign
 * metadata to match (objfile_section_align() unconditionally
 * early-returned for any section < SEC_NUM, a guard written only for
 * the separate, dynamically-registered `.section NAME` case). A linker
 * merging this object's `.rodata` with others (or simply placing it in
 * the final binary) had no reason to start the *section itself* at a
 * matching boundary, so the label's in-file alignment vanished:
 * `readelf -S` kept reporting sh_addralign=1 for the final `.rodata`
 * no matter what alignment was requested inside it.
 *
 * Found via a real OpenSSL build: crypto/bn/asm/x86_64-mont5.pl issues
 * `.section .rodata` + `.align 64` before its `.Linc` constant table,
 * which bn_mul_mont_gather5 loads via 16-byte-alignment-only MOVDQA
 * instructions -- the final linked `.rodata` section's sh_addralign
 * stayed 1, `.Linc` landed on an unaligned address, and every MOVDQA
 * against it segfaulted (OpenSSL's own exptest/bntest crashed with
 * SIGSEGV inside bn_mul_mont_gather5).
 *
 * This test assembles a real .s file requesting 64-byte `.rodata`
 * alignment and confirms both the final section header's sh_addralign
 * and the actual symbol address reflect it.
 */
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__APPLE__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_align_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_align_%d.o", td, pid);

    static const char src[] =
        ".text\n"
        ".globl gettbl\n"
        "gettbl:\n"
        "    leaq mytable(%rip), %rax\n"
        "    ret\n"
        ".section .rodata\n"
        ".align 64\n"
        "mytable:\n"
        ".long 0,0,1,1\n"
        ".long 2,2,2,2\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: assemble failed (rc=%d)\n", rc);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "readelf -SW %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    char out[4096] = {0};
    if (p) {
        size_t n = fread(out, 1, sizeof(out) - 1, p);
        out[n] = '\0';
        pclose(p);
    }

    char *line = strstr(out, ".rodata");
    if (!line) {
        printf("FAIL: .rodata section not found:\n%s\n", out);
        remove(objf);
        return 1;
    }
    char *eol = strchr(line, '\n');
    if (eol) *eol = '\0';
    /* readelf -SW's last numeric column on the section's own summary
     * line is Al (sh_addralign); confirm it's not the pre-fix "1". */
    char *last_field = line;
    for (char *tok = strtok(line, " \t"); tok; tok = strtok(NULL, " \t"))
        last_field = tok;
    if (strcmp(last_field, "64") != 0) {
        printf("FAIL: expected .rodata sh_addralign=64, got '%s' (full row: %s)\n",
               last_field, eol ? line : line);
        remove(objf);
        return 1;
    }

    remove(objf);
    printf("OK .rodata section correctly reports sh_addralign=64\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
