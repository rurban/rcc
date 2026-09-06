/* GCC Bug #92220 - -Wconversion generates a false warning for modulo expression when the modulus has smaller type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92220
 */
/* { dg-do compile } */
/* { dg-options "-Wconversion" } */


#include <stdint.h>

int main ( void )
{
  volatile uint64_t a = 123;
  volatile uint16_t b = 2;
  // warning: conversion from 'long unsigned int' to 'uint16_t'
  // {aka 'short unsigned int'} may change value [-Wconversion]

  uint16_t result = a % b;

  return result;
}

// The reporter also tried a C++ template wrapper (ModuloToSmallerSize) to
// work around the false positive; that alternate is C++, not C, and is
// omitted here since this reproducer targets the C front end.
