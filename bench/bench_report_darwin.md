# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           76 |          672 |        748 |
| RCC -O1   |           88 |          647 |        735 |
| RCC -O2   |           75 |          647 |        722 |
| TCC       |           61 |          583 |        644 |
| GCC -O0   |           79 |          479 |        558 |
| GCC -O2   |          121 |          294 |        415 |
| Clang -O0 |           84 |          468 |        552 |
| Clang -O2 |           93 |          286 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1412 us
  parse       bench.c       :    213 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    168 us
  link        bench_rcc     :    546 us
  link        bench_rcc     :  76296 us

RCC -O1:
  preprocess  bench.c       :    726 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    299 us
  link        bench_o1      :  81046 us

RCC -O2:
  preprocess  bench.c       :   1771 us
  parse       bench.c       :    359 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     45 us
  codegen     bench.c       :    277 us
  link        bench_o2      :    177 us
  link        bench_o2      :  81893 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 321807 us
  parse       sqlite3.c     :  58073 us
  typecheck   sqlite3.c     :  14956 us
  codegen     sqlite3.c     : 132116 us
  link        sqlite3.so    :  16851 us

RCC -O1:
  preprocess  sqlite3.c     : 291692 us
  parse       sqlite3.c     :  54968 us
  typecheck   sqlite3.c     :  18288 us
  opt         sqlite3.c     :  21984 us
  codegen     sqlite3.c     : 116997 us
  link        sqlite3.so    :  15889 us

RCC -O2:
  preprocess  sqlite3.c     : 290467 us
  parse       sqlite3.c     :  46792 us
  typecheck   sqlite3.c     :  12966 us
  opt         sqlite3.c     : 164994 us
  codegen     sqlite3.c     : 133817 us
  link        sqlite3.so    :  21182 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       767 ms |
| RCC -O1   |       749 ms |
| RCC -O2   |       910 ms |
| TCC       |       129 ms |
| GCC -O0   |      1106 ms |
| GCC -O2   |     12061 ms |
| Clang -O0 |      1165 ms |
| Clang -O2 |     11790 ms |
