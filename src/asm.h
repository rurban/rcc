// SPDX-License-Identifier: LGPL-2.1-or-later
// Built-in assembler: convert rcc-generated assembly text to an object file.
// Supports the exact subset of ARM64 and x86-64 that rcc emits.
#ifndef ASM_H
#define ASM_H

// Assemble the assembly text file at `asm_path` and write an ELF or Mach-O
// object file to `obj_path`. Returns 0 on success, -1 on error.
int assemble_file(const char *asm_path, const char *obj_path);

#endif // ASM_H
