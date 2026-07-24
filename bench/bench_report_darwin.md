# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           69 |          690 |        759 |
| RCC -O1   |           56 |          607 |        663 |
| RCC -O2   |           56 |          611 |        667 |
| TCC       |           48 |          580 |        628 |
| GCC -O0   |           72 |          485 |        557 |
| GCC -O2   |           97 |          277 |        374 |
| Clang -O0 |           60 |          475 |        535 |
| Clang -O2 |           93 |          277 |        370 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    812 us
  parse       bench.c:    152 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    156 us
  link        bench_rcc:  53140 us

RCC -O1:
  preprocess  bench.c:    624 us
  parse       bench.c:    137 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    127 us
  link        bench_rcc_o1:  51636 us

RCC -O2:
  preprocess  bench.c:    693 us
  parse       bench.c:    125 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    126 us
  link        bench_rcc_o2:  54114 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 286267 us
  parse       sqlite3.c:  61302 us
  typecheck   sqlite3.c:  19603 us
  codegen     sqlite3.c:  52608 us

RCC -O1:
  preprocess  sqlite3.c: 245318 us
  parse       sqlite3.c:  63394 us
  typecheck   sqlite3.c:  16166 us
  opt         sqlite3.c:  19108 us
  codegen     sqlite3.c:  56281 us

RCC -O2:
  preprocess  sqlite3.c: 221723 us
  parse       sqlite3.c:  45120 us
  typecheck   sqlite3.c:  12329 us
  opt         sqlite3.c: 211924 us
  codegen     sqlite3.c:  98350 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       587 ms |
| RCC -O1   |       451 ms |
| RCC -O2   |       550 ms |
| TCC       |        86 ms |
| GCC -O0   |      1041 ms |
| GCC -O2   |     10498 ms |
| Clang -O0 |      1116 ms |
| Clang -O2 |     14822 ms |
