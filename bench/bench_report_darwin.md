# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          640 |        703 |
| RCC -O1   |           86 |          709 |        795 |
| RCC -O2   |           81 |          654 |        735 |
| TCC       |           54 |          588 |        642 |
| GCC -O0   |           84 |          480 |        564 |
| GCC -O2   |          126 |          293 |        419 |
| Clang -O0 |           62 |          484 |        546 |
| Clang -O2 |           98 |          292 |        390 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1161 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    152 us
  link        bench_rcc     :    284 us
  link        bench_rcc     :  70768 us

RCC -O1:
  preprocess  bench.c       :    793 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    130 us
  link        bench_o1      :    187 us
  link        bench_o1      :  64623 us

RCC -O2:
  preprocess  bench.c       :    700 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    114 us
  link        bench_o2      :  59929 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 275128 us
  parse       sqlite3.c     :  54457 us
  typecheck   sqlite3.c     :  13441 us
  codegen     sqlite3.c     :  96896 us
  link        sqlite3.so    :  15969 us

RCC -O1:
  preprocess  sqlite3.c     : 234325 us
  parse       sqlite3.c     :  53244 us
  typecheck   sqlite3.c     :  14707 us
  opt         sqlite3.c     :  22808 us
  codegen     sqlite3.c     : 120095 us
  link        sqlite3.so    :  17325 us

RCC -O2:
  preprocess  sqlite3.c     : 250398 us
  parse       sqlite3.c     :  50650 us
  typecheck   sqlite3.c     :  13270 us
  opt         sqlite3.c     : 140755 us
  codegen     sqlite3.c     :  95401 us
  link        sqlite3.so    :  16483 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       626 ms |
| RCC -O1   |       638 ms |
| RCC -O2   |       792 ms |
| TCC       |        99 ms |
| GCC -O0   |      1094 ms |
| GCC -O2   |     10862 ms |
| Clang -O0 |      1099 ms |
| Clang -O2 |     10937 ms |
