# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          162 |          732 |        894 |
| RCC -O1   |           92 |          664 |        756 |
| RCC -O2   |           90 |          810 |        900 |
| TCC       |           97 |          802 |        899 |
| GCC -O0   |          200 |          703 |        903 |
| GCC -O2   |          206 |          401 |        607 |
| Clang -O0 |          305 |          692 |        997 |
| Clang -O2 |          287 |          417 |        704 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1442 us
  parse       bench.c:    150 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    116 us
  link        bench_rcc:  77345 us

RCC -O1:
  preprocess  bench.c:    611 us
  parse       bench.c:    138 us
  typecheck   bench.c:      6 us
  opt         bench.c:     21 us
  codegen     bench.c:    127 us
  link        bench_rcc_o1:  61789 us

RCC -O2:
  preprocess  bench.c:    727 us
  parse       bench.c:    133 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    129 us
  link        bench_rcc_o2:  64178 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 455221 us
  parse       sqlite3.c: 216423 us
  typecheck   sqlite3.c:  37189 us
  codegen     sqlite3.c:  69472 us

RCC -O1:
  preprocess  sqlite3.c: 303693 us
  parse       sqlite3.c:  59256 us
  typecheck   sqlite3.c:  15010 us
  opt         sqlite3.c:  30617 us
  codegen     sqlite3.c:  93464 us

RCC -O2:
  preprocess  sqlite3.c: 422310 us
  parse       sqlite3.c: 111016 us
  typecheck   sqlite3.c:  25877 us
  opt         sqlite3.c: 241247 us
  codegen     sqlite3.c:  83818 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1222 ms |
| RCC -O1   |       862 ms |
| RCC -O2   |      1321 ms |
| TCC       |       249 ms |
| GCC -O0   |      2557 ms |
| GCC -O2   |     23670 ms |
| Clang -O0 |      1950 ms |
| Clang -O2 |     16993 ms |
