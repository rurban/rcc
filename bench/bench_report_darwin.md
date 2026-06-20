# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           84 |          609 |        693 |
| RCC -O1   |           86 |          606 |        692 |
| TCC       |           49 |          533 |        582 |
| GCC -O0   |           71 |          452 |        523 |
| GCC -O2   |          100 |          276 |        376 |
| Clang -O0 |           63 |          463 |        526 |
| Clang -O2 |           93 |          283 |        376 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1957 us
  lex         bench.c:     84 us
  parse       bench.c:    143 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1876 us
  peephole    bench.c:    291 us (est, 778 calls)
  link        bench_rcc:  78076 us

RCC -O1:
  preprocess  bench.c:   2662 us
  lex         bench.c:     85 us
  parse       bench.c:     93 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   1685 us
  peephole    bench.c:    290 us (est, 775 calls)
  link        bench_rcc_o1:  77054 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1900213 us
  lex         sqlite3.c:  77870 us
  parse       sqlite3.c:  82033 us
  typecheck   sqlite3.c:  21277 us
  codegen     sqlite3.c: 637360 us
  peephole    sqlite3.c: 203933 us (est, 543823 calls)
  link        null: 5246182 us

RCC -O1:
  preprocess  sqlite3.c: 763985 us
  lex         sqlite3.c:  73702 us
  parse       sqlite3.c:  60645 us
  typecheck   sqlite3.c:  14739 us
  opt(CTFE)   sqlite3.c:  18927 us
  codegen     sqlite3.c: 466713 us
  peephole    sqlite3.c: 202702 us (est, 540539 calls)
  link        null: 5107525 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6578 ms |
| RCC -O1   |      6622 ms |
| TCC       |        86 ms |
| GCC -O0   |       849 ms |
| GCC -O2   |      9190 ms |
| Clang -O0 |       874 ms |
| Clang -O2 |      9625 ms |
