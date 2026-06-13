// SPDX-License-Identifier: LGPL-2.1-or-later
// rcc library API — compile C source to a shared library in-process.
// Used by run_tests to eliminate per-test exec overhead under Wine.
#ifndef RCC_LIB_H
#define RCC_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RCCLib RCCLib;

// Create a new compiler instance.  Must be freed with rcc_lib_delete().
RCCLib *rcc_lib_new(void);

// Compile a single C source file to a shared library suitable for
// rcc_lib_get_symbol().  Returns 0 on success, non-zero on failure.
// The output path is chosen automatically (temp dir + basename).
int rcc_lib_compile_file(RCCLib *lib, const char *path);

// Compile a C source file with an additional include directory (may be
// NULL) and extra linker flags (e.g. "-lm", may be NULL).
int rcc_lib_compile_file_ex(RCCLib *lib, const char *path,
                            const char *include_dir,
                            const char *extra_link_flags);

// Compile C source text to a shared library.  The file must contain a
// complete translation unit.  Returns 0 on success.
int rcc_lib_compile_string(RCCLib *lib, const char *src);

// Return a function pointer for the named symbol (e.g. "main").
// Returns NULL if the symbol is not found.
void *rcc_lib_get_symbol(RCCLib *lib, const char *name);

// Return the absolute path of the compiled shared library, or NULL
// before compilation.
const char *rcc_lib_output_path(const RCCLib *lib);

// Free the compiler instance and unload any compiled shared library.
// Temporary files are removed.
void rcc_lib_delete(RCCLib *lib);

#ifdef __cplusplus
}
#endif

#endif // RCC_LIB_H
