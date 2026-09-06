/* GCC Bug #8294 - Support another archetype in "format" function attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=8294
 */
/* { dg-do compile } */

// A common way to write vararg functions is to have the function take a
// count of the number of arguments that follow.  (Let's call this, say, the
// "enumerated" format, just so I can refer to it below.)  It would be nice
// if calls to this kind of function were checked by gcc to have the right
// number of arguments.  This could be a variant of the "format" attribute,
// like the following:

void foo(int arg_count, ...)
    __attribute__ ((format (enumerated(arg_count)))); /* { dg-error "wrong number of arguments specified for .format. attribute" } */

// In fact, even better would be if one could specify any linear function;
// for example, if the number of arguments should be 2*arg_count+1, then
// perhaps it could be declared like so:

void foo(int arg_count, ...)
    __attribute__ ((format (enumerated(arg_count, 2, 1)))); /* { dg-error "wrong number of arguments specified for .format. attribute" } */

// This would enable it to handle cases I have seen that do data base
// queries, where the number of key-value pairs is passed:
//    result_t query(char *table_name, int key_count, ...)
//      __attribute__ ((format(enumerated(key_count, 2, 0))));
// I hope this is the right forum for this kind of feature request.
