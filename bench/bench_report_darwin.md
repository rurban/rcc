# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          129 |          904 |       1033 |
| RCC -O1   |          176 |          877 |       1053 |
| RCC -O2   |          131 |          973 |       1104 |
| TCC       |          122 |          858 |        980 |
| GCC -O0   |          168 |          687 |        855 |
| GCC -O2   |          378 |          387 |        765 |
| Clang -O0 |          123 |          748 |        871 |
| Clang -O2 |          207 |          394 |        601 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2938 us
  parse       bench.c       :    563 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    769 us
  link        bench_rcc     :    121 us
  link        bench_rcc     : 135600 us

RCC -O1:
  preprocess  bench.c       :   2040 us
  parse       bench.c       :   1015 us
  typecheck   bench.c       :     20 us
  opt         bench.c       :     60 us
  codegen     bench.c       :    406 us
  link        bench_o1      :   1035 us
  link        bench_o1      : 143296 us

RCC -O2:
  preprocess  bench.c       :   1209 us
  parse       bench.c       :    339 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    349 us
  link        bench_o2      :    917 us
  link        bench_o2      : 123731 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 753166 us
  parse       sqlite3.c     : 289088 us
  typecheck   sqlite3.c     :  41046 us
  codegen     sqlite3.c     : 260727 us
  link        sqlite3.so    :  33535 us

RCC -O1:
  preprocess  sqlite3.c     : 524309 us
  parse       sqlite3.c     :  80516 us
  typecheck   sqlite3.c     :  30887 us
  opt         sqlite3.c     : 642845 us
  codegen     sqlite3.c     : 709384 us
  link        sqlite3.so    :  34041 us

RCC -O2:
  preprocess  sqlite3.c     : 652187 us
  parse       sqlite3.c     : 135453 us
  typecheck   sqlite3.c     :  26979 us
  opt         sqlite3.c     : 459236 us
  codegen     sqlite3.c     : 199760 us
  link        sqlite3.so    :  27160 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1599 ms |
| RCC -O1   |      1783 ms |
| RCC -O2   |      2435 ms |
| TCC       |       302 ms |
| GCC -O0   |      2465 ms |
| GCC -O2   |     21057 ms |
| Clang -O0 |      2137 ms |
| Clang -O2 |     19883 ms |
