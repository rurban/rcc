/* GCC Bug #92773 - GCC compilation with big array / header is infinite
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92773
 */


typedef unsigned char cfg_u8;
typedef union {
 struct {
  cfg_u8 offset;
  cfg_u8 value;
 };

 struct {
  cfg_u8 command;
  cfg_u8 param;
 };
} cfg_reg;

#define FOO { 0x00, 0x00 },
#define FOO1 FOO FOO FOO FOO FOO FOO FOO FOO FOO
#define FOO2 FOO1 FOO1 FOO1 FOO1 FOO1 FOO1 FOO1 FOO1
cfg_reg tas5756m_init_hf3[] = { FOO2 };

// now repeat more initializers (the original testcase has plenty).


