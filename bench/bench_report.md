# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          159 |          645 |        804 |
| RCC -O1   |           47 |          782 |        829 |
| TCC       |           12 |          695 |        707 |
| SLIMCC    |           42 |          697 |        739 |
| KEFIR     |          241 |          851 |       1092 |
| KEFIR -O1 |          208 |          514 |        722 |
| CCC       |           50 |          779 |        829 |
| GCC -O0   |           74 |          601 |        675 |
| GCC -O2   |          184 |          249 |        433 |
| Clang -O0 |           93 |          855 |        948 |
| Clang -O2 |          138 |          235 |        373 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   5456 us
  lex         bench.c:    549 us
  parse       bench.c:    533 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    662 us
  link        bench_rcc:  44107 us

RCC -O1:
  preprocess  bench.c:   4141 us
  lex         bench.c:    408 us
  parse       bench.c:    564 us
  typecheck   bench.c:      9 us
  opt(CTFE)   bench.c:     24 us
  codegen     bench.c:    625 us
  link        bench_rcc_o1:  41210 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 203905 us
  lex         sqlite3.c: 132631 us
  parse       sqlite3.c: 178926 us
  typecheck   sqlite3.c:  18020 us
  codegen     sqlite3.c: 268541 us

RCC -O1:
  preprocess  sqlite3.c: 183225 us
  lex         sqlite3.c: 122666 us
  parse       sqlite3.c: 169338 us
  typecheck   sqlite3.c:  14624 us
  opt(CTFE)   sqlite3.c:  34147 us
  codegen     sqlite3.c: 247611 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1028 ms |
| RCC -O1   |       776 ms |
| TCC       |       130 ms |
| SLIMCC    |      1803 ms |
| KEFIR     |     27828 ms |
| KEFIR -O1 |     33356 ms |
| CCC       |     20666 ms |
| GCC -O0   |     12929 ms |
| GCC -O2   |     80864 ms |
| Clang -O0 |      3442 ms |
| Clang -O2 |     45430 ms |
