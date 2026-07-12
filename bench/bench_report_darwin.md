# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          663 |        714 |
| RCC -O1   |           64 |          628 |        692 |
| TCC       |           42 |          564 |        606 |
| GCC -O0   |           88 |          458 |        546 |
| GCC -O2   |          110 |          285 |        395 |
| Clang -O0 |           61 |          458 |        519 |
| Clang -O2 |          120 |          290 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    584 us
  lex         bench.c:     74 us
  parse       bench.c:     99 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    134 us
  link        bench_rcc:  49959 us

RCC -O1:
  preprocess  bench.c:    562 us
  lex         bench.c:     72 us
  parse       bench.c:    109 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    143 us
  link        bench_rcc_o1:  51414 us
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
| TCC       |       120 ms |
| GCC -O0   |      1196 ms |
| GCC -O2   |     13495 ms |
| Clang -O0 |      1533 ms |
| Clang -O2 |     13210 ms |
