# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          634 |        688 |
| RCC -O1   |           53 |          641 |        694 |
| RCC -O2   |           53 |          634 |        687 |
| TCC       |           43 |          565 |        608 |
| GCC -O0   |           81 |          470 |        551 |
| GCC -O2   |          110 |          284 |        394 |
| Clang -O0 |           56 |          465 |        521 |
| Clang -O2 |           88 |          279 |        367 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    713 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    120 us
  link        bench_rcc     :     98 us
  link        bench_rcc     :  48815 us

RCC -O1:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    113 us
  link        bench_o1      :  48020 us

RCC -O2:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    137 us
  link        bench_o2      :    128 us
  link        bench_o2      :  47938 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 232857 us
  parse       sqlite3.c     :  68323 us
  typecheck   sqlite3.c     :  13297 us
  codegen     sqlite3.c     :  97744 us
  link        sqlite3.so    :  16016 us

RCC -O1:
  preprocess  sqlite3.c     : 210232 us
  parse       sqlite3.c     :  49780 us
  typecheck   sqlite3.c     :  12540 us
  opt         sqlite3.c     : 136819 us
  codegen     sqlite3.c     :  96769 us
  link        sqlite3.so    :  15788 us

RCC -O2:
  preprocess  sqlite3.c     : 206077 us
  parse       sqlite3.c     :  54640 us
  typecheck   sqlite3.c     :  12867 us
  opt         sqlite3.c     : 155390 us
  codegen     sqlite3.c     :  97724 us
  link        sqlite3.so    :  15682 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       585 ms |
| RCC -O1   |       702 ms |
| RCC -O2   |       812 ms |
| TCC       |       100 ms |
| GCC -O0   |      1050 ms |
| GCC -O2   |      9687 ms |
| Clang -O0 |      1051 ms |
| Clang -O2 |     10454 ms |
