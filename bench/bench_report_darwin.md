# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           91 |          638 |        729 |
| RCC -O1   |           69 |          617 |        686 |
| TCC       |           45 |          552 |        597 |
| GCC -O0   |           88 |          463 |        551 |
| GCC -O2   |          138 |          281 |        419 |
| Clang -O0 |           76 |          473 |        549 |
| Clang -O2 |          125 |          285 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2763 us
  lex         bench.c:     94 us
  parse       bench.c:     97 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    190 us
  link        bench_rcc:  75660 us

RCC -O1:
  preprocess  bench.c:   3733 us
  lex         bench.c:    109 us
  parse       bench.c:    110 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     18 us
  codegen     bench.c:    295 us
  link        bench_rcc_o1:  61536 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1592755 us
  lex         sqlite3.c:  94200 us
  parse       sqlite3.c:  73251 us
  typecheck   sqlite3.c:  21378 us
  codegen     sqlite3.c: 2743679 us

RCC -O1:
  preprocess  sqlite3.c: 683850 us
  lex         sqlite3.c:  84300 us
  parse       sqlite3.c:  95438 us
  typecheck   sqlite3.c:  23338 us
  opt(CTFE)   sqlite3.c:  25473 us
  codegen     sqlite3.c: 2785058 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      4588 ms |
| RCC -O1   |      3787 ms |
| TCC       |        92 ms |
| GCC -O0   |      1010 ms |
| GCC -O2   |     10472 ms |
| Clang -O0 |      1001 ms |
| Clang -O2 |     10808 ms |
