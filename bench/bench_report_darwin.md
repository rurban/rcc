# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          128 |          536 |        664 |
| RCC -O1   |           54 |          582 |        636 |
| TCC       |           28 |            2 |         30 |
| GCC -O0   |          114 |          405 |        519 |
| GCC -O2   |          119 |          240 |        359 |
| Clang -O0 |           43 |          406 |        449 |
| Clang -O2 |           66 |          238 |        304 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1313 us
  lex         bench.c:    100 us
  parse       bench.c:    252 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   2955 us
  peephole    bench.c:    222 us
  link        bench_rcc: 298406 us

RCC -O1:
  preprocess  bench.c:    484 us
  lex         bench.c:     95 us
  parse       bench.c:     48 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1450 us
  peephole    bench.c:    201 us
  link        bench_rcc_o1:  48836 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 258966 us
  lex         sqlite3.c:  80416 us
  parse       sqlite3.c: 1026762 us
  typecheck   sqlite3.c:  12865 us
  codegen     sqlite3.c: 650612 us
  peephole    sqlite3.c: 149825 us
  link        null: 4662154 us

RCC -O1:
  preprocess  sqlite3.c: 240962 us
  lex         sqlite3.c:  84119 us
  parse       sqlite3.c: 1059627 us
  typecheck   sqlite3.c:  14539 us
  opt(CTFE)   sqlite3.c:  16244 us
  codegen     sqlite3.c: 704129 us
  peephole    sqlite3.c: 154811 us
  link        null: 4457330 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       6661ms |
| RCC -O1   |       6604ms |
| TCC       |          5ms |
| GCC -O0   |        760ms |
| GCC -O2   |       7532ms |
| Clang -O0 |        729ms |
| Clang -O2 |       7477ms |
