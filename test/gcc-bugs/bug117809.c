/* GCC Bug #117809 - feature request: attribute malloc but for non-function-return-value return values
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117809
 */
/* { dg-do compile } */

// This is a feature request: GCC's `malloc` attribute can only describe a
// function's own return value as freshly allocated memory; it has no way
// to describe an "out" pointer parameter (e.g. a `void **` result argument)
// as receiving freshly allocated memory.  The reporter's proposed syntax
// (illustrative only - GCC does not implement this attribute) would look
// like this:

typedef int errno_t;
void my_free (void *ptr);

/* Illustrative only - the proposed malloc2 attribute does not exist in
   gcc, so this must stay commented out: */
// __attribute__((malloc2(1,my_free,2)))
// errno_t my_malloc(void** ptr, size_t len);

// Here is a more real-world example, from the GNUNET interface to jansson (json lib):

typedef struct json_t json_t;

struct GNUNET_JSON_Specification
GNUNET_JSON_spec_json (const char *name,
                       json_t **jsonp);
// I'd like to be able to annotate the second argument.
