# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          877 |        939 |
| RCC -O1   |           62 |          860 |        922 |
| TCC       |           14 |          823 |        837 |
| SLIMCC    |           80 |          915 |        995 |
| KEFIR     |          291 |          896 |       1187 |
| KEFIR -O1 |          270 |          706 |        976 |
| CCC       |           71 |          801 |        872 |
| GCC -O0   |          105 |          814 |        919 |
| GCC -O2   |          276 |          306 |        582 |
| Clang -O0 |          153 |          612 |        765 |
| Clang -O2 |          142 |          227 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   4360 us
  lex         bench.c:    447 us
  parse       bench.c:    647 us
  typecheck   bench.c:     13 us
  codegen     bench.c:    580 us
  link        bench_rcc:  62440 us

RCC -O1:
  preprocess  bench.c:   4340 us
  lex         bench.c:    440 us
  parse       bench.c:    654 us
  typecheck   bench.c:     15 us
  opt(CTFE)   bench.c:     28 us
  codegen     bench.c:    683 us
  link        bench_rcc_o1:  53745 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 226571 us
  lex         sqlite3.c: 131693 us
  parse       sqlite3.c: 235272 us
  typecheck   sqlite3.c:  29696 us
  codegen     sqlite3.c: 197983 us

RCC -O1:
  preprocess  sqlite3.c: 209311 us
  lex         sqlite3.c: 125051 us
  parse       sqlite3.c: 228461 us
  typecheck   sqlite3.c:  28623 us
  opt(CTFE)   sqlite3.c:  63723 us
  codegen     sqlite3.c: 203431 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       543 ms |
| RCC -O1   |       600 ms |
| TCC       |       121 ms |
| SLIMCC    |      1240 ms |
| KEFIR     |     22850 ms |
| KEFIR -O1 |     25515 ms |
| CCC       |     17257 ms |
| GCC -O0   |     10176 ms |
| GCC -O2   |     67636 ms |
| Clang -O0 |      3018 ms |
| Clang -O2 |     39376 ms |
