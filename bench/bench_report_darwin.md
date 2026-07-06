# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           61 |          609 |        670 |
| RCC -O1   |           53 |          626 |        679 |
| TCC       |           40 |          561 |        601 |
| GCC -O0   |           72 |          478 |        550 |
| GCC -O2   |          105 |          284 |        389 |
| Clang -O0 |           57 |          438 |        495 |
| Clang -O2 |           76 |          270 |        346 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1417 us
  lex         bench.c:     85 us
  parse       bench.c:     90 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    169 us
  link        bench_rcc:  49534 us

RCC -O1:
  preprocess  bench.c:   2012 us
  lex         bench.c:     78 us
  parse       bench.c:     82 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    157 us
  link        bench_rcc_o1:  44556 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 798811 us
  lex         sqlite3.c:  73013 us
  parse       sqlite3.c:  73078 us
  typecheck   sqlite3.c:  19664 us
  codegen     sqlite3.c: 2597969 us

RCC -O1:
  preprocess  sqlite3.c: 672488 us
  lex         sqlite3.c:  63106 us
  parse       sqlite3.c:  42891 us
  typecheck   sqlite3.c:  13856 us
  opt(CTFE)   sqlite3.c:  18033 us
  codegen     sqlite3.c: 2640848 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3390 ms |
| RCC -O1   |      3265 ms |
| TCC       |        73 ms |
| GCC -O0   |       860 ms |
| GCC -O2   |      9100 ms |
| Clang -O0 |       899 ms |
| Clang -O2 |      9135 ms |
