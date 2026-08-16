# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          675 |        749 |
| RCC -O1   |           68 |          663 |        731 |
| RCC -O2   |           76 |          645 |        721 |
| TCC       |           50 |          552 |        602 |
| GCC -O0   |           71 |          465 |        536 |
| GCC -O2   |          106 |          278 |        384 |
| Clang -O0 |           62 |          471 |        533 |
| Clang -O2 |          100 |          279 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   3121 us
  parse       bench.c       :    258 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    226 us
  link        bench_rcc     :    255 us
  link        bench_rcc     : 100095 us

RCC -O1:
  preprocess  bench.c       :   1070 us
  parse       bench.c       :    255 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    321 us
  link        bench_o1      :    467 us
  link        bench_o1      :  89775 us

RCC -O2:
  preprocess  bench.c       :    856 us
  parse       bench.c       :    208 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    189 us
  link        bench_o2      :    237 us
  link        bench_o2      : 109042 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 328380 us
  parse       sqlite3.c     :  80405 us
  typecheck   sqlite3.c     :  13510 us
  codegen     sqlite3.c     : 108706 us
  link        sqlite3.so    :  15369 us

RCC -O1:
  preprocess  sqlite3.c     : 246675 us
  parse       sqlite3.c     :  55843 us
  typecheck   sqlite3.c     :  12062 us
  opt         sqlite3.c     : 149790 us
  codegen     sqlite3.c     : 154790 us
  link        sqlite3.so    :  15465 us

RCC -O2:
  preprocess  sqlite3.c     : 272729 us
  parse       sqlite3.c     :  53076 us
  typecheck   sqlite3.c     :  15208 us
  opt         sqlite3.c     : 140082 us
  codegen     sqlite3.c     : 140903 us
  link        sqlite3.so    :  22786 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       649 ms |
| RCC -O1   |       732 ms |
| RCC -O2   |       735 ms |
| TCC       |        96 ms |
| GCC -O0   |      1041 ms |
| GCC -O2   |     10468 ms |
| Clang -O0 |      1063 ms |
| Clang -O2 |     10119 ms |
