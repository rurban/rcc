# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           73 |          703 |        776 |
| RCC -O1   |           71 |          640 |        711 |
| RCC -O2   |           68 |          625 |        693 |
| TCC       |           48 |          554 |        602 |
| GCC -O0   |           83 |          477 |        560 |
| GCC -O2   |           99 |          350 |        449 |
| Clang -O0 |          175 |          579 |        754 |
| Clang -O2 |          131 |          324 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    764 us
  parse       bench.c:    132 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    140 us
  link        bench_rcc:  56840 us

RCC -O1:
  preprocess  bench.c:    675 us
  parse       bench.c:    123 us
  typecheck   bench.c:      5 us
  opt         bench.c:     22 us
  codegen     bench.c:    112 us
  link        bench_rcc_o1:  55398 us

RCC -O2:
  preprocess  bench.c:    549 us
  parse       bench.c:    117 us
  typecheck   bench.c:      4 us
  opt         bench.c:     24 us
  codegen     bench.c:    109 us
  link        bench_rcc_o2:  54201 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 273770 us
  parse       sqlite3.c:  49442 us
  typecheck   sqlite3.c:  11616 us
  codegen     sqlite3.c:  41904 us

RCC -O1:
  preprocess  sqlite3.c: 233677 us
  parse       sqlite3.c:  56878 us
  typecheck   sqlite3.c:  16482 us
  opt         sqlite3.c:  26242 us
  codegen     sqlite3.c:  55333 us

RCC -O2:
  preprocess  sqlite3.c: 213037 us
  parse       sqlite3.c:  42987 us
  typecheck   sqlite3.c:  12211 us
  opt         sqlite3.c: 132582 us
  codegen     sqlite3.c:  58661 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       815 ms |
| RCC -O1   |       724 ms |
| RCC -O2   |       855 ms |
| TCC       |        94 ms |
| GCC -O0   |      1209 ms |
| GCC -O2   |     11680 ms |
| Clang -O0 |      1136 ms |
| Clang -O2 |     12615 ms |
