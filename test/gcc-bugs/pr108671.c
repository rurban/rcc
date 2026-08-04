/* GCC Bug #108671 - spurious "defined but not used" warning with static call back function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108671
 */
/* { dg-do compile } */
/* { dg-options "-O -std=c99 -Wall -Wunused-function" } */

/* Self-contained reduction of the reporter's openssl callback setup:
 * static callbacks are passed to external functions, which used to
 * spurious -Wunused-function warnings.  Modern gcc does not warn. */
extern void set_id_cb(unsigned long (*cb)(void));
extern void set_lock_cb(void (*cb)(int, int, const char *, int));

static unsigned long openssl_id_callback(void)
{
  return 42;
}

static void openssl_locking_callback(int mode, int n, const char *file, int line)
{
  (void)mode; (void)n; (void)file; (void)line;
}

void setup(void)
{
  set_id_cb(openssl_id_callback);
  set_lock_cb(openssl_locking_callback);
}