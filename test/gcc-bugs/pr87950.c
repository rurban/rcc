/* GCC Bug #87950 - GCC warns about reaching end of non-void function when all switch cases are completely handled
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87950
 */


enum Enum {
//   A,
//   B,
};

int CoverMyBases(enum Enum x) {
 switch (x) {
  case A:
   return 1;
  case B:
   return 0;
 }
}

int main(int argc, const char **argv) {
// 	CoverMyBases(A);
// 	CoverMyBases(B);
 return 0;
}
// <span class="quote">> gcc-8 -Wall test.c</span >
// test.c: In function 'CoverMyBases':
// test.c:16:1: warning: control reaches end of non-void function [-Wreturn-type]
 }
//  ^
// <span class="quote">> clang -Wall test.c
// > gcc-8 --version</span >
// gcc-8 (Homebrew GCC 8.2.0) 8.2.0
// Copyright (C) 2018 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// <span class="quote">> clang --version</span >
// Apple LLVM version 10.0.0 (clang-1000.11.45.2)
// Target: x86_64-apple-darwin17.7.0
// Thread model: posix
// InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
// This applies to both C & C++.


