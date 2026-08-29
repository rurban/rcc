# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          651 |        715 |
| RCC -O1   |           64 |          661 |        725 |
| RCC -O2   |           72 |          657 |        729 |
| TCC       |           70 |          573 |        643 |
| GCC -O0   |           86 |          482 |        568 |
| GCC -O2   |          156 |          321 |        477 |
| Clang -O0 |           77 |          497 |        574 |
| Clang -O2 |          112 |          315 |        427 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1414 us
  parse       bench.c       :    430 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    208 us
  link        bench_rcc     :    622 us
  link        bench_rcc     :  93495 us

RCC -O1:
  preprocess  bench.c       :   1853 us
  parse       bench.c       :    395 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    323 us
  link        bench_o1      :    489 us
  link        bench_o1      :  84806 us

RCC -O2:
  preprocess  bench.c       :    816 us
  parse       bench.c       :    228 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    187 us
  link        bench_o2      :    461 us
  link        bench_o2      :  60887 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 364943 us
  parse       sqlite3.c     :  76606 us
  typecheck   sqlite3.c     :  14511 us
  codegen     sqlite3.c     : 147061 us
  link        sqlite3.so    :  19184 us

RCC -O1:
  preprocess  sqlite3.c     : 318932 us
  parse       sqlite3.c     :  75466 us
  typecheck   sqlite3.c     :  15505 us
  opt         sqlite3.c     : 196912 us
  codegen     sqlite3.c     : 119377 us
  link        sqlite3.so    :  18598 us

RCC -O2:
  preprocess  sqlite3.c     : 283126 us
  parse       sqlite3.c     :  64605 us
  typecheck   sqlite3.c     :  14414 us
  opt         sqlite3.c     : 212448 us
  codegen     sqlite3.c     : 127900 us
  link        sqlite3.so    :  17992 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       855 ms |
| RCC -O1   |       944 ms |
| RCC -O2   |       897 ms |
| TCC       |       107 ms |
| GCC -O0   |      1278 ms |
| GCC -O2   |     14309 ms |
| Clang -O0 |      1629 ms |
| Clang -O2 |     11959 ms |
