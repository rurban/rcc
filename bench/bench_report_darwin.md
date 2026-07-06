# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           65 |          584 |        649 |
| RCC -O1   |           48 |          572 |        620 |
| TCC       |           36 |          512 |        548 |
| GCC -O0   |           73 |          434 |        507 |
| GCC -O2   |           98 |          262 |        360 |
| Clang -O0 |           49 |          438 |        487 |
| Clang -O2 |           76 |          262 |        338 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    988 us
  lex         bench.c:     74 us
  parse       bench.c:     78 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    138 us
  link        bench_rcc:  43268 us

RCC -O1:
  preprocess  bench.c:   1230 us
  lex         bench.c:     76 us
  parse       bench.c:     77 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     19 us
  codegen     bench.c:    119 us
  link        bench_rcc_o1:  41423 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 965765 us
  lex         sqlite3.c:  74114 us
  parse       sqlite3.c:  52222 us
  typecheck   sqlite3.c:  17163 us
  codegen     sqlite3.c: 2717736 us

RCC -O1:
  preprocess  sqlite3.c: 574787 us
  lex         sqlite3.c:  76660 us
  parse       sqlite3.c:  65407 us
  typecheck   sqlite3.c:  19665 us
  opt(CTFE)   sqlite3.c:  21503 us
  codegen     sqlite3.c: 2547718 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3297 ms |
| RCC -O1   |      3257 ms |
| TCC       |        78 ms |
| GCC -O0   |       826 ms |
| GCC -O2   |      9379 ms |
| Clang -O0 |       845 ms |
| Clang -O2 |      9066 ms |
