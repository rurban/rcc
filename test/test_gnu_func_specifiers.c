/* Two related declaration-parsing gaps hit while building the Linux
 * kernel with rcc:
 *
 * - declarator_params must handle "(void)" params followed directly by a
 *   GCC specifier (no intervening declarator), instead of falling through
 *   into logic that expects a real parameter list to continue.
 *
 * - An unknown GCC function specifier of the shape ident(...) between the
 *   closing ')' of the parameter list and ';' (e.g. the kernel's
 *   __cond_acquires(true, lock)) must be consumed as an opaque balanced-
 *   paren token run at the declaration site, not rejected.
 *
 * Function *definitions* ({ body) with trailing specifiers are a
 * separate, still-open gap — this only covers declarations.
 */

extern int decl_with_specifier(void) __cond_acquires(true, some_lock);

int main(void)
{
    return 0;
}
