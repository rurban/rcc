/* GCC Bug #117809 - feature request: attribute malloc but for non-function-return-value return values
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117809
 */


__attribute__((malloc2(1,my_free,2)))
errno_t my_malloc(void** ptr, size_t len);
// Here is a more real-world example, from the GNUNET interface to jansson (json lib):

struct GNUNET_JSON_Specification
// GNUNET_JSON_spec_json (const char *name,
                       json_t **jsonp);
// I'd like to be able to annotate the second argument.


