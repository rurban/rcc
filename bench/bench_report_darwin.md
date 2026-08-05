# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           81 |          715 |        796 |
| RCC -O1   |           80 |          766 |        846 |
| RCC -O2   |          116 |          660 |        776 |
| TCC       |          107 |          568 |        675 |
| GCC -O0   |           93 |          481 |        574 |
| GCC -O2   |          110 |          279 |        389 |
| Clang -O0 |           70 |          495 |        565 |
| Clang -O2 |          106 |          299 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1163 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    122 us
  link        bench_rcc     :     69 us
  link        bench_rcc     :  48248 us

RCC -O1:
  preprocess  bench.c       :    734 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    113 us
  link        bench_o1      :    127 us
  link        bench_o1      :  48275 us

RCC -O2:
  preprocess  bench.c       :    527 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    104 us
  link        bench_o2      :    389 us
  link        bench_o2      :  47696 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 255287 us
  parse       sqlite3.c     :  86093 us
  typecheck   sqlite3.c     :  19147 us
  codegen     sqlite3.c     : 111067 us
  link        sqlite3.so    :  13540 us

RCC -O1:
  preprocess  sqlite3.c     : 271620 us
  parse       sqlite3.c     :  48923 us
  typecheck   sqlite3.c     :  12033 us
  opt         sqlite3.c     :  19483 us
  codegen     sqlite3.c     : 131899 us
  link        sqlite3.so    :  16246 us

RCC -O2:
  preprocess  sqlite3.c     : 273376 us
  parse       sqlite3.c     :  51205 us
  typecheck   sqlite3.c     :  12095 us
  opt         sqlite3.c     : 156473 us
  codegen     sqlite3.c     : 144859 us
  link        sqlite3.so    :  15861 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       884 ms |
| RCC -O1   |       666 ms |
| RCC -O2   |       864 ms |
| TCC       |       119 ms |
| GCC -O0   |      1410 ms |
| GCC -O2   |     10155 ms |
| Clang -O0 |      1060 ms |
| Clang -O2 |      9333 ms |
