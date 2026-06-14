# Linux RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          882 |        974 |
| RCC -O1   |           85 |          874 |        959 |
| TCC       |           14 |          834 |        848 |
| SLIMCC    |           77 |          928 |       1005 |
| KEFIR     |          288 |          961 |       1249 |
| KEFIR -O1 |          291 |          486 |        777 |
| GCC -O0   |           84 |          556 |        640 |
| GCC -O2   |          182 |          213 |        395 |
| Clang -O0 |           94 |          625 |        719 |
| Clang -O2 |          138 |          232 |        370 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    584 us
  lex         bench.c:    365 us
  parse       bench.c:    567 us
  typecheck   bench.c:     14 us
  codegen     bench.c:   1325 us
  peephole    bench.c:    468 us
  link        bench_rcc:  85138 us

RCC -O1:
  preprocess  bench.c:    989 us
  lex         bench.c:   1970 us
  parse       bench.c:   1481 us
  typecheck   bench.c:     32 us
  opt(CTFE)   bench.c:     48 us
  codegen     bench.c:   3164 us
  peephole    bench.c:    721 us
  link        bench_rcc_o1:  98155 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 326140 us
  lex         sqlite3.c: 184988 us
  parse       sqlite3.c: 827344 us
  typecheck   sqlite3.c:  27422 us
  codegen     sqlite3.c: 524397 us
  peephole    sqlite3.c: 217474 us
  link        null: 183965 us

RCC -O1:
  preprocess  sqlite3.c: 327758 us
  lex         sqlite3.c: 183480 us
  parse       sqlite3.c: 840673 us
  typecheck   sqlite3.c:  28218 us
  opt(CTFE)   sqlite3.c:  63267 us
  codegen     sqlite3.c: 754689 us
  peephole    sqlite3.c: 290786 us
  link        null: 2432056 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       1661ms |
| RCC -O1   |       3169ms |
| TCC       |        114ms |
| SLIMCC    |        460ms |
| GCC -O0   |      10363ms |
| GCC -O2   |      69814ms |
| Clang -O0 |       3284ms |
| Clang -O2 |      45010ms |
