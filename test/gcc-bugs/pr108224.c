/* GCC Bug #108224 - Suggest stdlib.h header for rand
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108224
 */


{"getenv", {"<stdlib.h>", "<cstdlib>"} },
     {"malloc", {"<stdlib.h>", "<cstdlib>"} },
     {"realloc", {"<stdlib.h>", "<cstdlib>"} },
+    {"random", {"<stdlib.h>", "<cstdlib>"} },
+    {"srandom", {"<stdlib.h>", "<cstdlib>"} },
+    {"initstate", {"<stdlib.h>", "<cstdlib>"} },
+    {"setstate", {"<stdlib.h>", "<cstdlib>"} },

//      /* <string.h> and <cstring>.  */
     {"memchr", {"<string.h>", "<cstring>"} },
// -- 
// 2.37.2


