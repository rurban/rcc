// SPDX-License-Identifier: LGPL-2.1-or-later
// Shared helpers for unit-test driver programs (test/test_*.c): locating a
// writable scratch directory, detecting the qemu-aarch64 cross-test runner,
// and locating the rcc binary to exec against.
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

// Portable directory creation.  POSIX mkdir takes a mode argument; the
// Windows CRT only declares the 1-arg `_mkdir`.  Tests calling the
// 2-arg form directly no longer compile under rcc's (correct, gcc-
// matching) function-call argument-count check, so route every test
// through this wrapper.
static int test_mkdir(const char *path) {
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

static const char *get_tmpdir(void) {
#ifdef _WIN32
    static char buf[512];
    const char *tmp = getenv("TEMP");
    if (!tmp || !*tmp) tmp = getenv("TMP");
    if (!tmp || !*tmp) tmp = ".";
    snprintf(buf, sizeof(buf), "%s", tmp);
    return buf;
#else
    const char *tmp = getenv("TMPDIR");
    return (tmp && *tmp) ? tmp : "/tmp";
#endif
}

static int under_aarch64_qemu(void) {
    return access("/proc/sys/fs/binfmt_misc/qemu-aarch64", F_OK) == 0;
}

static const char *find_rcc(void) {
    const char *env = getenv("RCC");
    if (env && access(env, X_OK) == 0)
    return env;
#ifdef _WIN32
    return "rcc.exe";
#elif defined(__aarch64__)
    if (under_aarch64_qemu() && access("./rcc-arm64", X_OK) == 0)
        return "./rcc-arm64";
    return "./rcc";
#else
    return "./rcc";
#endif
}

// The shell redirection that discards stderr. There is no /dev/null on
// native Windows — cmd.exe's own redirect setup fails trying to open that
// path, aborting the whole system()/popen() command before it even runs
// (this is masked under Wine, whose Z: drive maps the host filesystem
// root, so /dev/null there resolves to the real device).
#ifdef _WIN32
#define NULL_REDIRECT "2>NUL"
#else
#define NULL_REDIRECT "2>/dev/null"
#endif

// The PC-relative (rel32) relocation type name objdump prints for a
// R_X86_64_PC32-shaped fixup — platform-dependent, since ELF and PE/COFF
// use entirely different relocation type namespaces for the same thing.
static const char *pc32_reloc_name(void) {
#ifdef _WIN32
    return "IMAGE_REL_AMD64_REL32";
#else
    return "R_X86_64_PC32";
#endif
}

// The object-file section name rcc emits read-only data into — ".rodata"
// under ELF, but ".rdata" under PE/COFF (GNU as's own mingw convention),
// even though the GAS source spells it ".rodata" on both.
static const char *rodata_section_name(void) {
#ifdef _WIN32
    return ".rdata";
#else
    return ".rodata";
#endif
}

// Different objdump builds format the raw-hex-byte column of `-d` output
// differently -- some space-separate each byte ("66 0f 6e"), some run them
// together ("660f6e") -- purely a display choice, not a semantic one; the
// encoded bytes are identical either way. Strip spaces in-place so tests
// that grep disassembly for a raw byte sequence match regardless of which
// objdump happens to be first on PATH.
static void strip_spaces(char *s) {
    char *w = s;
    for (char *r = s; *r; r++)
        if (*r != ' ') *w++ = *r;
    *w = '\0';
}

#endif // TEST_COMMON_H
