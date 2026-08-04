/* GCC Bug #103104 - missing warning about superfluous forward declaration -Wsuperfluous-forward-declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103104
 */


static void add_env_var_paths (const char *, incpath_kind);
static void add_standard_paths (const char *, const char *, const char *, int);
static void free_path (struct cpp_dir *, int);
static void merge_include_chains (const char *, cpp_reader *, int);
static void add_sysroot_to_chain (const char *, int);
static struct cpp_dir *remove_duplicates (cpp_reader *, struct cpp_dir *,
       struct cpp_dir *, struct cpp_dir *,
// --
static void
// free_path (struct cpp_dir *path, int reason)
// --
static void
// add_env_var_paths (const char *env_var, incpath_kind chain)
// --
static void
// add_standard_paths (const char *sysroot, const char *iprefix,
// --
static struct cpp_dir *
// remove_duplicates (cpp_reader *pfile, struct cpp_dir *head,
// --
// ...
// All definitions are in topologically correct order, i.e. defined before used, each and every forward declaration is redundant and error prone.
// Maybe there is an existing plugin to that effect?


