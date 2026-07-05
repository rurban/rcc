# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          583 |        649 |
| RCC -O1   |           52 |          572 |        624 |
| TCC       |           37 |          512 |        549 |
| GCC -O0   |           81 |          435 |        516 |
| GCC -O2   |           98 |          262 |        360 |
| Clang -O0 |           53 |          434 |        487 |
| Clang -O2 |           78 |          263 |        341 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1682 us
  lex         bench.c:     76 us
  parse       bench.c:     85 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    130 us
  link        bench_rcc:  46162 us

RCC -O1:
  preprocess  bench.c:   1249 us
  lex         bench.c:     73 us
  parse       bench.c:     72 us
  typecheck   bench.c:      3 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    128 us
  link        bench_rcc_o1:  43077 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1315744 us
  lex         sqlite3.c:  87214 us
  parse       sqlite3.c:  87803 us
  typecheck   sqlite3.c:  20084 us
  codegen     sqlite3.c: 2527512 us

RCC -O1:
  preprocess  sqlite3.c: 767990 us
  lex         sqlite3.c:  69072 us
  parse       sqlite3.c:  48303 us
  typecheck   sqlite3.c:  13603 us
  opt(CTFE)   sqlite3.c:  18200 us
  codegen     sqlite3.c: 2526814 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3552 ms |
| RCC -O1   |      3599 ms |
| TCC       |        88 ms |
| GCC -O0   |       974 ms |
| GCC -O2   |      8711 ms |
| Clang -O0 |       847 ms |
| Clang -O2 |      8391 ms |
