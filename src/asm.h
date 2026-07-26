// SPDX-License-Identifier: LGPL-2.1-or-later
// Built-in assembler: convert rcc-generated assembly text to an object file.
// Supports the exact subset of ARM64 and x86-64 that rcc emits.
#ifndef ASM_H
#define ASM_H

#include "obj.h"

// Assemble the assembly text file at `asm_path` and write an ELF or Mach-O
// object file to `obj_path`. Returns 0 on success, -1 on error.
int assemble_file(const char *asm_path, const char *obj_path);

// Called by assemble_inline for each forward-reference fixup it cannot
// resolve itself. `patch_off` is the byte offset within section `section`
// (an ObjFile section index — SEC_TEXT/SEC_DATA/... or SEC_NUM+N for a
// dynamically-registered .pushsection'd section) of the 4-byte relative
// displacement placeholder; `label` is the target label name.
typedef void (*inline_fixup_fn)(size_t patch_off, int section, const char *label, void *ctx);

// Assemble `tmpl` (newline-separated AT&T instructions) directly into
// `obj`'s text section. Resolved fixups are patched immediately; unresolved
// ones are reported via `on_forward` (may be NULL). Returns 0 on success.
int assemble_inline(ObjFile *obj, const char *tmpl,
                    inline_fixup_fn on_forward, void *ctx);

#endif // ASM_H
