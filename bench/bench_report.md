# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           46 |          600 |        646 |
| RCC -O1   |           41 |          599 |        640 |
| TCC       |           10 |          567 |        577 |
| SLIMCC    |           53 |          629 |        682 |
| KEFIR     |          195 |          660 |        855 |
| KEFIR -O1 |          202 |          500 |        702 |
| CCC       |           38 |          573 |        611 |
| GCC -O0   |           71 |          582 |        653 |
| GCC -O2   |          172 |          216 |        388 |
| Clang -O0 |          105 |          635 |        740 |
| Clang -O2 |          148 |          231 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   3187 us
  lex         bench.c:    326 us
  parse       bench.c:    378 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    588 us
  link        bench_rcc:  33971 us

RCC -O1:
  preprocess  bench.c:   4194 us
  lex         bench.c:    439 us
  parse       bench.c:    354 us
  typecheck   bench.c:      9 us
  opt(CTFE)   bench.c:     22 us
  codegen     bench.c:    472 us
  link        bench_rcc_o1:  31279 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 205690 us
  lex         sqlite3.c: 112272 us
  parse       sqlite3.c: 163829 us
  typecheck   sqlite3.c:  13478 us
  codegen     sqlite3.c: 5426781 us

RCC -O1:
  preprocess  sqlite3.c: 213871 us
  lex         sqlite3.c: 112921 us
  parse       sqlite3.c: 151151 us
  typecheck   sqlite3.c:  13332 us
  opt(CTFE)   sqlite3.c:  30040 us
  codegen     sqlite3.c: 5517617 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5931 ms |
| RCC -O1   |      5965 ms |
| TCC       |       118 ms |
| SLIMCC    |      1289 ms |
| KEFIR     |     24338 ms |
| KEFIR -O1 |     26330 ms |
| CCC       |     17770 ms |
| GCC -O0   |     10506 ms |
| GCC -O2   |     68881 ms |
| Clang -O0 |      3165 ms |
| Clang -O2 |     44198 ms |
