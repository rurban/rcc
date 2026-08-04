/* GCC Bug #8294 - Support another archetype in "format" function attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=8294
 */


void foo(int arg_count, ...)
    __attribute__ ((format (enumerated(arg_count))));

// In fact, even better would be if one could specify any linear function; for example, if the number of arguments should be 2*arg_count+1, then perhaps it could be declared like so:

  void foo(int arg_count, ...)
    __attribute__ ((format (enumerated(arg_count, 2, 1))));
// This would enable it to handle cases I have seen that do data base queries, where the number of key-value pairs is passed:
//    result_t query(char *table_name, int key_count, ...)
     __attribute__ ((format(enumerated(key_count, 2, 0))));
// I hope this is the right forum for this kind of feature request.
Release:
// unknown


