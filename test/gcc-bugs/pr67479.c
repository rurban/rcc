/* GCC Bug #67479 - Support for -Wformat-pedantic
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67479
 */


return 0;
     }
//    *format = fcp + 1;
// -  if (pedantic && !dollar_format_warned)
// +  if (warn_format_pedantic && !dollar_format_warned)
     {
// -      warning (OPT_Wformat_, "%s does not support %%n$ operand number formats",
// -              C_STD_NAME (STD_EXT));
// -      dollar_format_warned = 1;
// +      dollar_format_warned =
// +       pedwarn (input_location, OPT_Wformat_pedantic,
// +                "%s does not support %%n$ operand number formats",
// +                C_STD_NAME (STD_EXT));
     }
   if (overflow_flag || argnum == 0
//        || (dollar_first_arg_num && argnum > dollar_arguments_count))
// Since 'pedantic' should only be used in the cases described in the guidelines (<a href="https://gcc.gnu.org/wiki/DiagnosticsGuidelines">https://gcc.gnu.org/wiki/DiagnosticsGuidelines</a>) and "if(pedantic) warning()" is not one of them. (This means that every use of pedantic in c-format.c is currently a minor bug).
// Fixing this will also contribute towards fixing <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - -Werror=pedantic should be equivalent to -pedantic-errors"
//    href="show_bug.cgi?id=53075">PR53075</a>.


