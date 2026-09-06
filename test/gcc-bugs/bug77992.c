/* GCC Bug #77992 - Provide feature to initialize padding bytes to avoid information leaks
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77992
 */


#include <string.h>
 #include <stdint.h>
 #include <stdio.h>

 struct struct_with_padding {
  uint32_t a;
  uint64_t b;
  uint32_t c;
 };
 int main()
 {
  struct struct_with_padding s;
// 		memset(&s, 0xff, sizeof(s));
		s = (struct struct_with_padding) {
			.a = 0xaaaaaaaa,
			.b = 0xbbbbbbbbbbbbbbbb,
 #ifdef SHOW_GCC_BUG
			.c = 0xdddddddd,
 #else
// 	//        .c = 0xdddddddd,
 #endif
  };

// 		uint8_t* p8 = (uint8_t*)(&s);

// 		printf("data: ");
		for (size_t i = 0; i < sizeof(s); ++i)
  {
// 			printf("0x%02x ", p8[i]);
  }
// 		printf("\n");
  return 0;
 }
// With `.c = 0xdddddddd`, the example output:
//     data: 0xaa 0xaa 0xaa 0xaa 0xff 0xff 0xff 0xff 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xdd 0xdd 0xdd 0xdd 0xff 0xff 0xff 0xff
// Without that, the example output:
//     data: 0xaa 0xaa 0xaa 0xaa 0x00 0x00 0x00 0x00 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
// So it looks like GCC could be improved by initializing the padding bytes in both cases?


