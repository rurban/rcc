# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          584 |        631 |
| RCC -O1   |           47 |          581 |        628 |
| RCC -O2   |           51 |          582 |        633 |
| TCC       |           40 |          512 |        552 |
| GCC -O0   |           65 |          433 |        498 |
| GCC -O2   |          106 |          263 |        369 |
| Clang -O0 |           50 |          434 |        484 |
| Clang -O2 |           79 |          265 |        344 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    893 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    120 us
  link        bench_rcc     :    184 us
  link        bench_rcc     :  48832 us

RCC -O1:
  preprocess  bench.c       :    547 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    109 us
  link        bench_o1      :    144 us
  link        bench_o1      :  43351 us

RCC -O2:
  preprocess  bench.c       :    531 us
  parse       bench.c       :    119 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    113 us
  link        bench_o2      :    173 us
  link        bench_o2      :  42308 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 199794 us
  parse       sqlite3.c     :  40945 us
  typecheck   sqlite3.c     :  11295 us
  codegen     sqlite3.c     :  83043 us
  link        sqlite3.so    :  13567 us

RCC -O1:
  preprocess  sqlite3.c     : 187676 us
  parse       sqlite3.c     :  40188 us
  typecheck   sqlite3.c     :  11923 us
  opt         sqlite3.c     :  17662 us
  codegen     sqlite3.c     :  82236 us
  link        sqlite3.so    :  13553 us

RCC -O2:
  preprocess  sqlite3.c     : 182393 us
  parse       sqlite3.c     :  40222 us
  typecheck   sqlite3.c     :  11287 us
  opt         sqlite3.c     : 123116 us
  codegen     sqlite3.c     :  82260 us
  link        sqlite3.so    :  16144 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       634 ms |
| RCC -O1   |       706 ms |
| RCC -O2   |       643 ms |
| TCC       |        91 ms |
| GCC -O0   |       912 ms |
| GCC -O2   |      9015 ms |
| Clang -O0 |       968 ms |
| Clang -O2 |      8557 ms |
