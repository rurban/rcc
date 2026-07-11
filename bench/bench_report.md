# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          633 |        691 |
| RCC -O1   |           52 |          628 |        680 |
| TCC       |           11 |          604 |        615 |
| SLIMCC    |           52 |          643 |        695 |
| KEFIR     |          231 |          725 |        956 |
| KEFIR -O1 |          246 |          508 |        754 |
| CCC       |           55 |          594 |        649 |
| GCC -O0   |           78 |          601 |        679 |
| GCC -O2   |          278 |          222 |        500 |
| Clang -O0 |          115 |          650 |        765 |
| Clang -O2 |          177 |          237 |        414 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   3362 us
  lex         bench.c:    363 us
  parse       bench.c:    407 us
  typecheck   bench.c:      8 us
  codegen     bench.c:    627 us
  link        bench_rcc:  37287 us

RCC -O1:
  preprocess  bench.c:   3462 us
  lex         bench.c:    360 us
  parse       bench.c:    424 us
  typecheck   bench.c:      8 us
  opt(CTFE)   bench.c:     19 us
  codegen     bench.c:    545 us
  link        bench_rcc_o1:  32631 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 246957 us
  lex         sqlite3.c: 116012 us
  parse       sqlite3.c: 171712 us
  typecheck   sqlite3.c:  15406 us
  codegen     sqlite3.c: 5855357 us

RCC -O1:
  preprocess  sqlite3.c: 231750 us
  lex         sqlite3.c: 115547 us
  parse       sqlite3.c: 159766 us
  typecheck   sqlite3.c:  13818 us
  opt(CTFE)   sqlite3.c:  30463 us
  codegen     sqlite3.c: 5678571 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6450 ms |
| RCC -O1   |      6669 ms |
| TCC       |       119 ms |
| SLIMCC    |      1382 ms |
| KEFIR     |     25768 ms |
| KEFIR -O1 |     42196 ms |
| CCC       |     32080 ms |
| GCC -O0   |     16452 ms |
| GCC -O2   |    147776 ms |
| Clang -O0 |      6449 ms |
| Clang -O2 |     84883 ms |
