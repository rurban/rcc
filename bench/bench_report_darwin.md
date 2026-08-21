# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           48 |          595 |        643 |
| RCC -O1   |           48 |          603 |        651 |
| RCC -O2   |           47 |          598 |        645 |
| TCC       |           37 |          515 |        552 |
| GCC -O0   |           57 |          437 |        494 |
| GCC -O2   |          103 |          264 |        367 |
| Clang -O0 |           52 |          434 |        486 |
| Clang -O2 |          105 |          303 |        408 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    643 us
  parse       bench.c       :    274 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :    302 us
  link        bench_rcc     :  47972 us

RCC -O1:
  preprocess  bench.c       :    646 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    153 us
  link        bench_o1      :    336 us
  link        bench_o1      :  44411 us

RCC -O2:
  preprocess  bench.c       :    570 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    135 us
  link        bench_o2      :    154 us
  link        bench_o2      :  43688 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 191323 us
  parse       sqlite3.c     :  46589 us
  typecheck   sqlite3.c     :  13089 us
  codegen     sqlite3.c     :  86364 us
  link        sqlite3.so    :  13772 us

RCC -O1:
  preprocess  sqlite3.c     : 177205 us
  parse       sqlite3.c     :  43682 us
  typecheck   sqlite3.c     :  11555 us
  opt         sqlite3.c     : 119069 us
  codegen     sqlite3.c     :  83784 us
  link        sqlite3.so    :  15496 us

RCC -O2:
  preprocess  sqlite3.c     : 173721 us
  parse       sqlite3.c     :  45236 us
  typecheck   sqlite3.c     :  11783 us
  opt         sqlite3.c     : 122244 us
  codegen     sqlite3.c     :  85183 us
  link        sqlite3.so    :  14442 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       871 ms |
| RCC -O1   |       895 ms |
| RCC -O2   |       895 ms |
| TCC       |       102 ms |
| GCC -O0   |      1497 ms |
| GCC -O2   |     12528 ms |
| Clang -O0 |      1301 ms |
| Clang -O2 |      9272 ms |
