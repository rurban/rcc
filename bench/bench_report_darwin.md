# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          136 |          619 |        755 |
| RCC -O1   |           79 |          614 |        693 |
| TCC       |           63 |          556 |        619 |
| GCC -O0   |           91 |          459 |        550 |
| GCC -O2   |          127 |          286 |        413 |
| Clang -O0 |           79 |          473 |        552 |
| Clang -O2 |          120 |          290 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1433 us
  lex         bench.c:     94 us
  parse       bench.c:    124 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    177 us
  link        bench_rcc:  50684 us

RCC -O1:
  preprocess  bench.c:   1770 us
  lex         bench.c:     77 us
  parse       bench.c:     80 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    137 us
  link        bench_rcc_o1:  47190 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1305589 us
  lex         sqlite3.c: 101169 us
  parse       sqlite3.c: 100943 us
  typecheck   sqlite3.c:  26193 us
  codegen     sqlite3.c: 2780554 us

RCC -O1:
  preprocess  sqlite3.c: 792481 us
  lex         sqlite3.c:  81230 us
  parse       sqlite3.c:  75437 us
  typecheck   sqlite3.c:  20174 us
  opt(CTFE)   sqlite3.c:  23188 us
  codegen     sqlite3.c: 2703068 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      4587 ms |
| RCC -O1   |      4261 ms |
| TCC       |        76 ms |
| GCC -O0   |      1072 ms |
| GCC -O2   |      9675 ms |
| Clang -O0 |      1055 ms |
| Clang -O2 |      9397 ms |
