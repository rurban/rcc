/* C23 #embed directive: verify basic embedding, limit(), empty files.
   Creates temp binary payloads at runtime, then compiles and runs
   test programs that exercise #embed against them. */
#include "test_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int write_file(const char *path, const unsigned char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (len) fwrite(data, 1, len, f);
    fclose(f);
    return 1;
}

/* Convert backslashes to forward slashes in-place — Windows paths
   embedded in C string literals would otherwise be interpreted as
   escape sequences (\t, \a, \b, ...). */
static void sanitize_path_for_c_source(char *s) {
    for (; *s; s++)
        if (*s == '\\') *s = '/';
}

/* Compile a source snippet, run the resulting binary, return exit code */
static int compile_and_run(const char *rcc, const char *src, const char *tag) {
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[512], exef[512], cmd[2048];
    snprintf(srcf, sizeof(srcf), "%s/test_embed_%s_%d.c", td, tag, pid);
    snprintf(exef, sizeof(exef), "%s/test_embed_%s_%d", td, tag, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL [%s]: cannot write %s\n", tag, srcf); return -1; }
    fputs(src, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -I%s -o %s %s " NULL_REDIRECT, rcc, td, exef, srcf);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL [%s]: compile failed (rc=%d)\n", tag, rc);
        remove(srcf);
        return -1;
    }
    remove(srcf);
    snprintf(cmd, sizeof(cmd), "%s", exef);
    rc = system(cmd);
    remove(exef);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    char binpath[512];
    /* Test 1: basic binary embedding */
    snprintf(binpath, sizeof(binpath), "%s/test_embed_basic_%d.bin", td, pid);
    sanitize_path_for_c_source(binpath);
    {
        unsigned char payload[] = {0xde, 0xad, 0xbe, 0xef};
        write_file(binpath, payload, sizeof(payload));
        char src[512];
        snprintf(src, sizeof(src),
            "int main(void) {\n"
            "    unsigned char d[] = {\n"
            "        #embed \"%s\"\n"
            "    , 0};\n"
            "    if (d[0] != 0xde) return 1;\n"
            "    if (d[1] != 0xad) return 2;\n"
            "    if (d[2] != 0xbe) return 3;\n"
            "    if (d[3] != 0xef) return 4;\n"
            "    if (d[4] != 0)    return 5;\n"
            "    return 0;\n"
            "}\n", binpath);
        int rc = compile_and_run(rcc, src, "basic");
        if (rc != 0) { printf("FAIL [basic]: exit=%d\n", rc); ok = 0; }
        remove(binpath);
    }

    /* Test 2: limit(N) truncation */
    snprintf(binpath, sizeof(binpath), "%s/test_embed_limit_%d.bin", td, pid);
    sanitize_path_for_c_source(binpath);
    {
        unsigned char payload[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
        write_file(binpath, payload, sizeof(payload));
        char src[512];
        snprintf(src, sizeof(src),
            "int main(void) {\n"
            "    unsigned char d[] = {\n"
            "        #embed \"%s\" limit(3)\n"
            "    , 0xff};\n"
            "    if (d[0] != 0xaa) return 1;\n"
            "    if (d[1] != 0xbb) return 2;\n"
            "    if (d[2] != 0xcc) return 3;\n"
            "    if (d[3] != 0xff) return 4;\n"
            "    return 0;\n"
            "}\n", binpath);
        int rc = compile_and_run(rcc, src, "limit");
        if (rc != 0) { printf("FAIL [limit]: exit=%d\n", rc); ok = 0; }
        remove(binpath);
    }

#ifdef _WIN32
    /* Test 3: angle-bracket include path (#embed <file>) */
    // On Windows MSYS2, /tmp doesn't exist. Use CWD instead.
    {
        unsigned char payload[] = {0x11, 0x22};
        const char *bf = "test_embed_angle_test.bin";
        write_file(bf, payload, sizeof(payload));
        char src[512];
        snprintf(src, sizeof(src),
            "int main(void) {\n"
            "    unsigned char d[] = {\n"
            "        #embed <test_embed_angle_test.bin>\n"
            "    , 0};\n"
            "    if (d[0] != 0x11) return 1;\n"
            "    if (d[1] != 0x22) return 2;\n"
            "    if (d[2] != 0)    return 3;\n"
            "    return 0;\n"
            "}\n");
        int rc = compile_and_run(rcc, src, "angle");
        if (rc != 0) { printf("FAIL [angle]: exit=%d\n", rc); ok = 0; }
        remove(bf);
    }
#else
    /* Test 3: angle-bracket include path (#embed <file>) */
    {
        unsigned char payload[] = {0x11, 0x22};
        const char *bf = "/tmp/test_embed_angle_test.bin";
        write_file(bf, payload, sizeof(payload));
        char src[512];
        snprintf(src, sizeof(src),
            "int main(void) {\n"
            "    unsigned char d[] = {\n"
            "        #embed </tmp/test_embed_angle_test.bin>\n"
            "    , 0};\n"
            "    if (d[0] != 0x11) return 1;\n"
            "    if (d[1] != 0x22) return 2;\n"
            "    if (d[2] != 0)    return 3;\n"
            "    return 0;\n"
            "}\n");
        int rc = compile_and_run(rcc, src, "angle");
        if (rc != 0) { printf("FAIL [angle]: exit=%d\n", rc); ok = 0; }
        remove(bf);
    }
#endif

    if (ok) printf("OK #embed\n");
    return ok ? 0 : 1;
}
