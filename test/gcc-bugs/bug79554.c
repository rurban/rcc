/* GCC Bug #79554 - Zero length format string passed to fprintf under if statement causes error message
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79554
 */
/* { dg-do compile } */


#define PRINT_CHANGE(fmt, args...) \
    do { \
//         fprintf(DEBUG_STREAM, "%s(", __FUNCTION__);  \
        if (strcmp(fmt, "") != 0) { \
//             fprintf(DEBUG_STREAM, fmt_, ##args); \
        } \
//         fprintf(DEBUG_STREAM, ")\n"); \
    } while (0)

// PRINT_CHANGE("");
// Example 2:
#define PRINT_CHANGE(fmt, args...) \
    do { \
//         fprintf(DEBUG_STREAM, "%s(", __FUNCTION__);  \
        const char *fmt_ = fmt;  \
        if (strcmp(fmt_, "") != 0) { \
//             fprintf(DEBUG_STREAM, fmt_, ##args); \
        } \
//         fprintf(DEBUG_STREAM, ")\n"); \
    } while (0)

// PRINT_CHANGE("");


