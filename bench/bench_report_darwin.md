# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          143 |          891 |       1034 |
| RCC -O1   |          110 |          852 |        962 |
| RCC -O2   |           98 |          862 |        960 |
| TCC       |           81 |          754 |        835 |
| GCC -O0   |          114 |          613 |        727 |
| GCC -O2   |          188 |          370 |        558 |
| Clang -O0 |          124 |          641 |        765 |
| Clang -O2 |          152 |          374 |        526 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          289 |         6964 |       7253 |
| RCC -O1   |          388 |         7047 |       7435 |
| RCC -O2   |          617 |         5400 |       6017 |
| TCC       |          173 |         5528 |       5701 |
| GCC -O0   |          727 |         4139 |       4866 |
| GCC -O2   |         1024 |         2467 |       3491 |
| Clang -O0 |          693 |         4369 |       5062 |
| Clang -O2 |         1162 |         2551 |       3713 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    897 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :    269 us
  link        bench_rcc     :  79571 us

RCC -O1:
  preprocess  bench.c       :    858 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    151 us
  link        bench_o1      :    232 us
  link        bench_o1      :  71930 us

RCC -O2:
  preprocess  bench.c       :    823 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    142 us
  link        bench_o2      :    366 us
  link        bench_o2      : 129938 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 470497 us
  parse       sqlite3.c     : 224133 us
  typecheck   sqlite3.c     :  24354 us
  codegen     sqlite3.c     : 208830 us
  link        sqlite3.so    :  45473 us

RCC -O1:
  preprocess  sqlite3.c     : 371340 us
  parse       sqlite3.c     :  87377 us
  typecheck   sqlite3.c     :  24693 us
  opt         sqlite3.c     : 307967 us
  codegen     sqlite3.c     : 242769 us
  link        sqlite3.so    :  21264 us

RCC -O2:
  preprocess  sqlite3.c     : 358603 us
  parse       sqlite3.c     :  95087 us
  typecheck   sqlite3.c     :  20288 us
  opt         sqlite3.c     : 283444 us
  codegen     sqlite3.c     : 195920 us
  link        sqlite3.so    :  16857 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1572 ms |
| RCC -O1   |      1151 ms |
| RCC -O2   |      1077 ms |
| TCC       |       165 ms |
| GCC -O0   |      1738 ms |
| GCC -O2   |     17435 ms |
| Clang -O0 |      1922 ms |
| Clang -O2 |     15357 ms |
