/* GCC Bug #108671 - spurious "defined but not used" warning with static call back function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108671
 */
/* { dg-do compile } */


static call back function used in openssl. Other examples online show people using static functions defined in the same file, so that is normal usage.
// cc -O -g -std=c99 -Wall -Wformat -Wimplicit -Wreturn-type -Wuninitialized -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -DMACHTYPE_x86_64 -DUSE_HIC   -Wall -Wformat -Wimplicit -Wreturn-type -Wuninitialized -I../inc -I../../inc -I../../../inc -I../../../../inc -I../../../../../inc -I../htslib  -I/include -I/usr/include/libpng16  -o https.o -c https.c
// https.c:46:13: warning: ‘openssl_locking_callback’ defined but not used [-Wunused-function]
//    46 | static void openssl_locking_callback(int mode, int n, const char * file, int line)
//       |             ^~~~~~~~~~~~~~~~~~~~~~~~
// https.c:41:22: warning: ‘openssl_id_callback’ defined but not used [-Wunused-function]
//    41 | static unsigned long openssl_id_callback(void)
//       |                      ^~~~~~~~~~~~~~~~~~~
// =====================
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

static unsigned long openssl_id_callback(void)
{
return ((unsigned long)pthread_self());
}

static void openssl_locking_callback(int mode, int n, const char * file, int line)
{
if (mode & CRYPTO_LOCK)
//     pthread_mutex_lock(&mutexes[n]);
// else
//     pthread_mutex_unlock(&mutexes[n]);
}

void openssl_pthread_setup(void)
{
int i;
int numLocks = CRYPTO_num_locks();
// AllocArray(mutexes, numLocks);
for (i = 0;  i < numLocks;  i++)
//     pthread_mutex_init(&mutexes[i], NULL);
// CRYPTO_set_id_callback(openssl_id_callback);
// CRYPTO_set_locking_callback(openssl_locking_callback);
}

void openSslInit()
// /* do only once */
{
static boolean done = FALSE;
static pthread_mutex_t osiMutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_lock( &osiMutex );
if (!done)
    {
//     SSL_library_init();
//     ERR_load_crypto_strings();
//     ERR_load_SSL_strings();
//     OpenSSL_add_all_algorithms();

//     openssl_pthread_setup();

    myDataIndex = SSL_get_ex_new_index(0, "myDataIndex", NULL, NULL, NULL);
    done = TRUE;
    }
// pthread_mutex_unlock( &osiMutex );
}
// So the two callback functions are not called directly,
// but they are passed to openssl functions that will call
// the functions later.
// We have been using this same code for many years up through gcc 4.85.
// Now with 4.9 we are seeing this warning for the first time.


