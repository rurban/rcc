# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          881 |        968 |
| RCC -O1   |          135 |          675 |        810 |
| RCC -O2   |           79 |          671 |        750 |
| TCC       |           56 |          569 |        625 |
| GCC -O0   |           88 |          504 |        592 |
| GCC -O2   |          140 |          292 |        432 |
| Clang -O0 |           60 |          499 |        559 |
| Clang -O2 |          145 |          310 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1587 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    203 us
  link        bench_rcc     :    309 us
  link        bench_rcc     :  92307 us

RCC -O1:
  preprocess  bench.c       :    762 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    146 us
  link        bench_o1      :     92 us
  link        bench_o1      :  73416 us

RCC -O2:
  preprocess  bench.c       :    753 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    144 us
  link        bench_o2      :    241 us
  link        bench_o2      :  59220 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 379880 us
  parse       sqlite3.c     : 189258 us
  typecheck   sqlite3.c     :  23712 us
  codegen     sqlite3.c     : 115948 us
  link        sqlite3.so    :  15793 us

RCC -O1:
  preprocess  sqlite3.c     : 237248 us
  parse       sqlite3.c     :  51598 us
  typecheck   sqlite3.c     :  13554 us
  opt         sqlite3.c     : 141786 us
  codegen     sqlite3.c     : 122700 us
  link        sqlite3.so    :  17957 us

RCC -O2:
  preprocess  sqlite3.c     : 244869 us
  parse       sqlite3.c     :  52331 us
  typecheck   sqlite3.c     :  13029 us
  opt         sqlite3.c     : 146594 us
  codegen     sqlite3.c     : 107542 us
  link        sqlite3.so    :  16959 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1290 ms |
| RCC -O1   |       860 ms |
| RCC -O2   |       835 ms |
| TCC       |       138 ms |
| GCC -O0   |      1281 ms |
| GCC -O2   |     16181 ms |
| Clang -O0 |      1772 ms |
| Clang -O2 |     17374 ms |
