/* GCC Bug #65445 - Improve [-W...] display for -Wformat
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65445
 */


static int opts_enable_opt_table [] = {
// ...
// 2, OPT_Wall, OPT_Wc++-compat, /* OPT_Wenum_compare */
// 1, OPT_Wformat,               /* OPT_Wformat_contains_nul */
// ...
};

static int opts_enable_opt_index [] = {
// ...
// /* OPT_Wenum_compare */ 50,
// /* OPT_Wformat_contains_nul */ 53,
// /* OPT_Wall */ -1,
// ....
};

// such that opts_enable_opt_table[opts_enable_opt_index[OPT_Wformat_contains_nul] + 1] == OPT_Wformat
// Then add a function that given an OPT_Wx, returns which OPT_Wy options that enable it are enabled at this moment. Then, use this function when printing the warning in diagnostics.c.


