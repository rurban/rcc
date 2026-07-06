# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          150 |          624 |        774 |
| RCC -O1   |           69 |          613 |        682 |
| TCC       |           54 |          551 |        605 |
| GCC -O0   |           83 |          465 |        548 |
| GCC -O2   |          105 |          283 |        388 |
| Clang -O0 |           65 |          466 |        531 |
| Clang -O2 |           93 |          280 |        373 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2201 us
  lex         bench.c:     84 us
  parse       bench.c:    104 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    162 us
  link        bench_rcc:  63763 us

RCC -O1:
  preprocess  bench.c:   2452 us
  lex         bench.c:     91 us
  parse       bench.c:    115 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    164 us
  link        bench_rcc_o1:  60491 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1890339 us
  lex         sqlite3.c:  88812 us
  parse       sqlite3.c:  85286 us
  typecheck   sqlite3.c:  26105 us
  codegen     sqlite3.c: 2791054 us

RCC -O1:
  preprocess  sqlite3.c: 1010397 us
  lex         sqlite3.c:  80096 us
  parse       sqlite3.c:  70768 us
  typecheck   sqlite3.c:  19737 us
  opt(CTFE)   sqlite3.c:  20261 us
  codegen     sqlite3.c: 2664437 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      3743 ms |
| RCC -O1   |      3901 ms |
| TCC       |       109 ms |
| GCC -O0   |      1014 ms |
| GCC -O2   |      9873 ms |
| Clang -O0 |      1010 ms |
| Clang -O2 |     10439 ms |
