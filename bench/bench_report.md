# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          599 |        648 |
| RCC -O1   |           42 |          594 |        636 |
| TCC       |           17 |          589 |        606 |
| SLIMCC    |           54 |          637 |        691 |
| KEFIR     |          206 |          678 |        884 |
| KEFIR -O1 |          205 |          495 |        700 |
| CCC       |           64 |          568 |        632 |
| GCC -O0   |           82 |          570 |        652 |
| GCC -O2   |          187 |          211 |        398 |
| Clang -O0 |          270 |          655 |        925 |
| Clang -O2 |          139 |          237 |        376 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   4137 us
  lex         bench.c:    415 us
  parse       bench.c:    463 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    639 us
  link        bench_rcc:  35973 us

RCC -O1:
  preprocess  bench.c:   3152 us
  lex         bench.c:    311 us
  parse       bench.c:    354 us
  typecheck   bench.c:      8 us
  opt(CTFE)   bench.c:     21 us
  codegen     bench.c:    477 us
  link        bench_rcc_o1:  35140 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 236810 us
  lex         sqlite3.c: 113208 us
  parse       sqlite3.c: 153360 us
  typecheck   sqlite3.c:  14170 us
  codegen     sqlite3.c: 5234351 us

RCC -O1:
  preprocess  sqlite3.c: 231081 us
  lex         sqlite3.c: 115285 us
  parse       sqlite3.c: 144145 us
  typecheck   sqlite3.c:  13371 us
  opt(CTFE)   sqlite3.c:  36026 us
  codegen     sqlite3.c: 5278447 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5984 ms |
| RCC -O1   |      5933 ms |
| TCC       |       126 ms |
| SLIMCC    |      1299 ms |
| KEFIR     |     23976 ms |
| KEFIR -O1 |     26405 ms |
| CCC       |     17748 ms |
| GCC -O0   |     10555 ms |
| GCC -O2   |     68362 ms |
| Clang -O0 |      3104 ms |
| Clang -O2 |     38098 ms |
