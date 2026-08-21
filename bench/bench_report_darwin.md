# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          595 |        655 |
| RCC -O1   |           50 |          601 |        651 |
| RCC -O2   |           56 |          626 |        682 |
| TCC       |          104 |          515 |        619 |
| GCC -O0   |           62 |          436 |        498 |
| GCC -O2   |          136 |          284 |        420 |
| Clang -O0 |           61 |          435 |        496 |
| Clang -O2 |           84 |          262 |        346 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    589 us
  parse       bench.c       :    116 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    106 us
  link        bench_rcc     :    356 us
  link        bench_rcc     :  43856 us

RCC -O1:
  preprocess  bench.c       :    549 us
  parse       bench.c       :    111 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    109 us
  link        bench_o1      :    151 us
  link        bench_o1      :  44308 us

RCC -O2:
  preprocess  bench.c       :    550 us
  parse       bench.c       :    116 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    108 us
  link        bench_o2      :    197 us
  link        bench_o2      :  45326 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 206434 us
  parse       sqlite3.c     :  75935 us
  typecheck   sqlite3.c     :  16299 us
  codegen     sqlite3.c     :  87864 us
  link        sqlite3.so    :  13537 us

RCC -O1:
  preprocess  sqlite3.c     : 179865 us
  parse       sqlite3.c     :  52197 us
  typecheck   sqlite3.c     :  13980 us
  opt         sqlite3.c     : 132499 us
  codegen     sqlite3.c     :  84025 us
  link        sqlite3.so    :  13553 us

RCC -O2:
  preprocess  sqlite3.c     : 172936 us
  parse       sqlite3.c     :  44406 us
  typecheck   sqlite3.c     :  11736 us
  opt         sqlite3.c     : 122316 us
  codegen     sqlite3.c     :  84881 us
  link        sqlite3.so    :  14452 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       682 ms |
| RCC -O1   |       662 ms |
| RCC -O2   |       642 ms |
| TCC       |        98 ms |
| GCC -O0   |       934 ms |
| GCC -O2   |      9525 ms |
| Clang -O0 |      1181 ms |
| Clang -O2 |      8623 ms |
