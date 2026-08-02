# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          187 |          677 |        864 |
| RCC -O1   |           61 |          640 |        701 |
| RCC -O2   |           56 |          630 |        686 |
| TCC       |           42 |          554 |        596 |
| GCC -O0   |           64 |          470 |        534 |
| GCC -O2   |           97 |          285 |        382 |
| Clang -O0 |           59 |          483 |        542 |
| Clang -O2 |          116 |          289 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    655 us
  parse       bench.c:    135 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    132 us
  link bench_rcc:    156 us
  link        bench_rcc:  56574 us

RCC -O1:
  preprocess  bench.c:    630 us
  parse       bench.c:    126 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    115 us
  link        bench_o1:   124 us
  link        bench_o1: 54652 us

RCC -O2:
  preprocess  bench.c:    692 us
  parse       bench.c:    131 us
  typecheck   bench.c:      4 us
  opt         bench.c:     22 us
  codegen     bench.c:    126 us
  link        bench_o2:   150 us
  link        bench_o2: 54963 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 297021 us
  parse       sqlite3.c:  56042 us
  typecheck   sqlite3.c:  16042 us
  codegen     sqlite3.c: 101526 us
  link    libsqlite3.so:  34014 us

RCC -O1:
  preprocess  sqlite3.c: 268545 us
  parse       sqlite3.c:  55508 us
  typecheck   sqlite3.c:  14439 us
  opt         sqlite3.c:  21467 us
  codegen     sqlite3.c: 106855 us
  link    libsqlite3.so:  15170 us

RCC -O2:
  preprocess  sqlite3.c: 247757 us
  parse       sqlite3.c:  52342 us
  typecheck   sqlite3.c:  12980 us
  opt         sqlite3.c: 180668 us
  codegen     sqlite3.c: 113213 us
  link    libsqlite3.so:  17897 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       732 ms |
| RCC -O1   |       633 ms |
| RCC -O2   |       866 ms |
| TCC       |       102 ms |
| GCC -O0   |      1095 ms |
| GCC -O2   |     10072 ms |
| Clang -O0 |      1170 ms |
| Clang -O2 |     10223 ms |
