# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          662 |        736 |
| RCC -O1   |           55 |          615 |        670 |
| RCC -O2   |           55 |          615 |        670 |
| TCC       |           42 |          544 |        586 |
| GCC -O0   |           65 |          461 |        526 |
| GCC -O2   |           94 |          275 |        369 |
| Clang -O0 |           54 |          460 |        514 |
| Clang -O2 |           89 |          276 |        365 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    551 us
  parse       bench.c:    113 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    102 us
  link        bench_rcc:  49432 us

RCC -O1:
  preprocess  bench.c:    617 us
  parse       bench.c:    126 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    117 us
  link        bench_rcc_o1:  49956 us

RCC -O2:
  preprocess  bench.c:    561 us
  parse       bench.c:    113 us
  typecheck   bench.c:      4 us
  opt         bench.c:     20 us
  codegen     bench.c:    113 us
  link        bench_rcc_o2:  48349 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 237099 us
  parse       sqlite3.c:  41316 us
  typecheck   sqlite3.c:  10979 us
  codegen     sqlite3.c:  45830 us

RCC -O1:
  preprocess  sqlite3.c: 231164 us
  parse       sqlite3.c:  49871 us
  typecheck   sqlite3.c:  13334 us
  opt         sqlite3.c:  21197 us
  codegen     sqlite3.c:  50910 us

RCC -O2:
  preprocess  sqlite3.c: 198180 us
  parse       sqlite3.c:  49131 us
  typecheck   sqlite3.c:  12748 us
  opt         sqlite3.c: 136720 us
  codegen     sqlite3.c:  48710 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       497 ms |
| RCC -O1   |       519 ms |
| RCC -O2   |       664 ms |
| TCC       |        76 ms |
| GCC -O0   |       992 ms |
| GCC -O2   |      9654 ms |
| Clang -O0 |       993 ms |
| Clang -O2 |      9331 ms |
