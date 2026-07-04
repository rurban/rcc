# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          591 |        646 |
| RCC -O1   |           46 |          588 |        634 |
| TCC       |           29 |          521 |        550 |
| GCC -O0   |           57 |          434 |        491 |
| GCC -O2   |           78 |          263 |        341 |
| Clang -O0 |           49 |          449 |        498 |
| Clang -O2 |           78 |          264 |        342 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    946 us
  lex         bench.c:     73 us
  parse       bench.c:     72 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    128 us
  link        bench_rcc:  40914 us

RCC -O1:
  preprocess  bench.c:   1070 us
  lex         bench.c:     72 us
  parse       bench.c:     73 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    120 us
  link        bench_rcc_o1:  40786 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 539864 us
  lex         sqlite3.c:  61549 us
  parse       sqlite3.c:  42740 us
  typecheck   sqlite3.c:  13309 us
  codegen     sqlite3.c: 2573233 us

RCC -O1:
  preprocess  sqlite3.c: 597965 us
  lex         sqlite3.c:  67373 us
  parse       sqlite3.c:  47209 us
  typecheck   sqlite3.c:  18057 us
  opt(CTFE)   sqlite3.c:  20579 us
  codegen     sqlite3.c: 2593646 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3129 ms |
| RCC -O1   |      3071 ms |
| TCC       |        67 ms |
| GCC -O0   |       797 ms |
| GCC -O2   |      8402 ms |
| Clang -O0 |       789 ms |
| Clang -O2 |      8158 ms |
