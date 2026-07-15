# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          654 |        733 |
| RCC -O1   |           56 |          587 |        643 |
| TCC       |           44 |          561 |        605 |
| GCC -O0   |          108 |          453 |        561 |
| GCC -O2   |          127 |          270 |        397 |
| Clang -O0 |           60 |          435 |        495 |
| Clang -O2 |           88 |          264 |        352 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    811 us
  lex         bench.c:     73 us
  parse       bench.c:    101 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    131 us
  link        bench_rcc:  45719 us

RCC -O1:
  preprocess  bench.c:    515 us
  lex         bench.c:     79 us
  parse       bench.c:     99 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    131 us
  link        bench_rcc_o1:  45593 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       105 ms |
| GCC -O0   |       909 ms |
| GCC -O2   |      9727 ms |
| Clang -O0 |      1100 ms |
| Clang -O2 |     10070 ms |
