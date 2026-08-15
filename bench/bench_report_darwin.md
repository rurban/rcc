# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          117 |          792 |        909 |
| RCC -O1   |          102 |          824 |        926 |
| RCC -O2   |          120 |          791 |        911 |
| TCC       |           84 |          662 |        746 |
| GCC -O0   |          108 |          548 |        656 |
| GCC -O2   |          160 |          298 |        458 |
| Clang -O0 |           63 |          504 |        567 |
| Clang -O2 |          109 |          319 |        428 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    768 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    127 us
  link        bench_rcc     :    123 us
  link        bench_rcc     :  65678 us

RCC -O1:
  preprocess  bench.c       :    797 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    367 us
  link        bench_o1      :  57454 us

RCC -O2:
  preprocess  bench.c       :    660 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    348 us
  link        bench_o2      :  52464 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 348788 us
  parse       sqlite3.c     : 203802 us
  typecheck   sqlite3.c     :  31714 us
  codegen     sqlite3.c     : 194469 us
  link        sqlite3.so    :  17166 us

RCC -O1:
  preprocess  sqlite3.c     : 335918 us
  parse       sqlite3.c     :  99817 us
  typecheck   sqlite3.c     :  40527 us
  opt         sqlite3.c     : 195451 us
  codegen     sqlite3.c     : 208259 us
  link        sqlite3.so    :  20425 us

RCC -O2:
  preprocess  sqlite3.c     : 342864 us
  parse       sqlite3.c     :  54633 us
  typecheck   sqlite3.c     :  15999 us
  opt         sqlite3.c     : 196034 us
  codegen     sqlite3.c     : 165773 us
  link        sqlite3.so    :  18830 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1371 ms |
| RCC -O1   |      1662 ms |
| RCC -O2   |      1519 ms |
| TCC       |       395 ms |
| GCC -O0   |      2671 ms |
| GCC -O2   |     16205 ms |
| Clang -O0 |      1714 ms |
| Clang -O2 |     14692 ms |
