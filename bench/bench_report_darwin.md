# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          628 |        680 |
| RCC -O1   |           57 |          602 |        659 |
| RCC -O2   |           61 |          679 |        740 |
| TCC       |           65 |          593 |        658 |
| GCC -O0   |           63 |          476 |        539 |
| GCC -O2   |           95 |          287 |        382 |
| Clang -O0 |           75 |          438 |        513 |
| Clang -O2 |           87 |          292 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    810 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    152 us
  link        bench_rcc     :    315 us
  link        bench_rcc     :  57273 us

RCC -O1:
  preprocess  bench.c       :    603 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    149 us
  link        bench_o1      :  49010 us

RCC -O2:
  preprocess  bench.c       :    730 us
  parse       bench.c       :    248 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    150 us
  link        bench_o2      :    135 us
  link        bench_o2      :  49113 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 273774 us
  parse       sqlite3.c     :  55341 us
  typecheck   sqlite3.c     :  12496 us
  codegen     sqlite3.c     :  94605 us
  link        sqlite3.so    :  14741 us

RCC -O1:
  preprocess  sqlite3.c     : 195722 us
  parse       sqlite3.c     :  47726 us
  typecheck   sqlite3.c     :  12412 us
  opt         sqlite3.c     : 127886 us
  codegen     sqlite3.c     : 105691 us
  link        sqlite3.so    :  17399 us

RCC -O2:
  preprocess  sqlite3.c     : 176867 us
  parse       sqlite3.c     :  44262 us
  typecheck   sqlite3.c     :  11660 us
  opt         sqlite3.c     : 124006 us
  codegen     sqlite3.c     :  89368 us
  link        sqlite3.so    :  15439 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       651 ms |
| RCC -O1   |       976 ms |
| RCC -O2   |       772 ms |
| TCC       |       133 ms |
| GCC -O0   |      1145 ms |
| GCC -O2   |     10967 ms |
| Clang -O0 |      1085 ms |
| Clang -O2 |     10520 ms |
