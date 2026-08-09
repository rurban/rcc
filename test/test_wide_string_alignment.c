/* Wide string literals (L"...") must be emitted 4-byte aligned (Linux/macOS
 * wchar_t is UTF-32) or 2-byte aligned (Windows UTF-16): glibc's/msvcrt's
 * wcslen()/wcscmp()/wmemcmp() are commonly vectorized, reading multiple
 * wchar_t at once under the assumption that any wchar_t object satisfies
 * _Alignof(wchar_t) -- true of every object a real compiler emits, string
 * literals included.
 *
 * Two separate bugs conspired to break this:
 *
 * 1. codegen.c's string-literal emission loop packed every StrLit (narrow
 *    and wide) back-to-back in .rodata with no padding, so a wide literal
 *    landed at whatever odd byte offset the preceding literal's length left
 *    behind.
 *
 * 2. Two call sites that register a string literal used purely for its
 *    ADDRESS -- read_global_label_initializer() (a global/const pointer
 *    initializer, e.g. `const wchar_t *p = L"text";`) and extract_reloc()'s
 *    ND_STR case (an `&expr` reloc extraction) -- hardcoded elem_size=1
 *    regardless of the literal's actual prefix, so even after fix #1 these
 *    specific literals were aligned (and packed) as if they were 1-byte
 *    narrow strings, silently corrupting their true 2/4-byte element size.
 *
 * A third, deeper bug meant fix #1 alone wasn't sufient for real multi-file
 * programs: elf_write.c hardcoded .rodata's ELF sh_addralign to 1 in every
 * .o it wrote, so even a *correctly* self-padded literal could still land
 * misaligned once the linker (rcc's own, or a real system ld) concatenated
 * this object's .rodata after another object's -- test-link.sh's own
 * "wide string alignment survives 2-TU link" case guards that half
 * separately, since it requires two real object files.
 *
 * Found via test/third_party/test_libarchive: LZ4IO_toHuman()-adjacent
 * archive_entry.c/archive_string.c wide-character setters (symlink/uname
 * fields) and the fileflags[] wcstofflags() parser table both read back
 * truncated/wrong lengths, corrupting archive_entry_copy_symlink_w() and
 * silently dropping bits from archive_entry_copy_fflags_text_w()'s parsed
 * flag set.
 */
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* Case 1: a local pointer initialized directly from a wide literal (the
 * ordinary value-expression path -- was already correct, kept as a
 * baseline/regression guard). */
static int check_local_pointer(void) {
    const wchar_t *p = L"symlinkname2";
    if (wcslen(p) != 12) {
        printf("FAIL: local pointer wcslen = %zu, expected 12\n", wcslen(p));
        return 0;
    }
    return 1;
}

/* Case 2: a global const array holding a NUL-prefixed wide string in
 * braces -- the exact LZ4IO_toHuman()/units[] shape, minus the local-init
 * brace-string bug (already covered by test_local_char_array_brace_strlit.c)
 * -- exercises the codegen.c padding fix directly. */
static const wchar_t units[] = L"\0KMGTPEZY";

static int check_units_table(void) {
    if (wcslen(units) != 0) { /* embedded NUL: wcslen stops at index 0 */
        printf("FAIL: units[0] wcslen should be 0 (embedded NUL), got %zu\n", wcslen(units));
        return 0;
    }
    /* The exact original bug: wcslen() past the embedded NUL, on a
     * misaligned literal, returned a wrong length (12 chars off by 1). */
    if (wcslen(units + 1) != 8) {
        printf("FAIL: wcslen(units+1) = %zu, expected 8\n", wcslen(units + 1));
        return 0;
    }
    static const wchar_t expect[] = {L'\0', L'K', L'M', L'G', L'T', L'P', L'E', L'Z', L'Y', L'\0'};
    if (memcmp(units, expect, sizeof(expect)) != 0) {
        printf("FAIL: units[] content mismatch past the embedded NUL\n");
        return 0;
    }
    if (units[2] != L'M') {
        printf("FAIL: units[2] = %d, expected 'M' (%d)\n", (int)units[2], (int)L'M');
        return 0;
    }
    return 1;
}

/* Case 3: a struct array whose field is a POINTER initialized directly
 * from a wide literal -- read_global_label_initializer()'s exact
 * hardcoded-elem_size=1 bug (the fileflags[] shape). */
struct flag {
    const char *name;
    const wchar_t *wname;
    unsigned long value;
};
static const struct flag fileflags[] = {
    {"nosappnd", L"nosappnd", 32},
    {"noschg", L"noschg", 16},
    {NULL, NULL, 0},
};

static int check_struct_field_pointers(void) {
    if (wcslen(fileflags[0].wname) != 8) {
        printf("FAIL: fileflags[0].wname wcslen = %zu, expected 8\n",
               wcslen(fileflags[0].wname));
        return 0;
    }
    if (wcslen(fileflags[1].wname) != 6) {
        printf("FAIL: fileflags[1].wname wcslen = %zu, expected 6\n",
               wcslen(fileflags[1].wname));
        return 0;
    }
    /* The "noXXXX" -> "XXXX" suffix match ae_wcstofflags() itself performs:
     * skip the "no" prefix and compare the remainder. */
    if (wmemcmp(L"sappnd", fileflags[0].wname + 2, 6) != 0) {
        printf("FAIL: wmemcmp against fileflags[0].wname+2 mismatched\n");
        return 0;
    }
    return 1;
}

/* Case 4: &L"literal" reached through extract_reloc()'s ND_STR case (an
 * address-of-string-literal expression used as a global pointer's
 * initializer), which previously re-registered a SECOND, wrongly-tagged
 * StrLit instead of reusing the correctly-tagged one primary() already
 * created. */
static const wchar_t *addr_of_literal = &(L"addressed")[0];

static int check_addr_of_literal(void) {
    if (wcslen(addr_of_literal) != 9) {
        printf("FAIL: addr_of_literal wcslen = %zu, expected 9\n",
               wcslen(addr_of_literal));
        return 0;
    }
    return 1;
}

/* Force every literal above to actually land somewhere non-trivial by
 * interleaving narrow (odd-byte-length) string literals ahead of them in
 * source order -- a narrow literal's own byte length is never a multiple
 * of 4, so without the fix at least one of the wide literals above would
 * be forced off a 4-byte boundary. */
static const char narrow_pad[] = "odd5";
static const char narrow_pad2[] = "seven!!";

int main(void) {
    (void)narrow_pad;
    (void)narrow_pad2;
    int ok = 1;
    ok &= check_local_pointer();
    ok &= check_units_table();
    ok &= check_struct_field_pointers();
    ok &= check_addr_of_literal();
    if (!ok) return 1;
    printf("ok\n");
    return 0;
}
