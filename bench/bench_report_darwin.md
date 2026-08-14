# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           82 |          647 |        729 |
| RCC -O1   |          126 |          696 |        822 |
| RCC -O2   |           73 |          642 |        715 |
| TCC       |           51 |          595 |        646 |
| GCC -O0   |          134 |          523 |        657 |
| GCC -O2   |          140 |          300 |        440 |
| Clang -O0 |           63 |          500 |        563 |
| Clang -O2 |          107 |          291 |        398 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    751 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    136 us
  link        bench_rcc     :     91 us
  link        bench_rcc     :  72574 us

RCC -O1:
  preprocess  bench.c       :    843 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    129 us
  link        bench_o1      :    305 us
  link        bench_o1      :  73248 us

RCC -O2:
  preprocess  bench.c       :    788 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    132 us
  link        bench_o2      :    271 us
  link        bench_o2      :  81143 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 432538 us
  parse       sqlite3.c     : 216320 us
  typecheck   sqlite3.c     :  33834 us
  codegen     sqlite3.c     : 130870 us
  link        sqlite3.so    :  23400 us

RCC -O1:
  preprocess  sqlite3.c     : 320672 us
  parse       sqlite3.c     :  81326 us
  typecheck   sqlite3.c     :  22472 us
  opt         sqlite3.c     : 161633 us
  codegen     sqlite3.c     : 117076 us
  link        sqlite3.so    :  19096 us

RCC -O2:
  preprocess  sqlite3.c     : 239065 us
  parse       sqlite3.c     :  50023 us
  typecheck   sqlite3.c     :  14315 us
  opt         sqlite3.c     : 144478 us
  codegen     sqlite3.c     : 109525 us
  link        sqlite3.so    :  17185 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1316 ms |
| RCC -O1   |       979 ms |
| RCC -O2   |       815 ms |
| TCC       |       126 ms |
| GCC -O0   |      1080 ms |
| GCC -O2   |     12615 ms |
| Clang -O0 |      1386 ms |
| Clang -O2 |     11376 ms |
