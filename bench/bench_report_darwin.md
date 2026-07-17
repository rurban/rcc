# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          616 |        673 |
| RCC -O1   |           68 |          685 |        753 |
| TCC       |           61 |          554 |        615 |
| GCC -O0   |           89 |          465 |        554 |
| GCC -O2   |          112 |          281 |        393 |
| Clang -O0 |           60 |          464 |        524 |
| Clang -O2 |           90 |          280 |        370 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    634 us
  lex         bench.c:     74 us
  parse       bench.c:    140 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    166 us
  link        bench_rcc:  59673 us

RCC -O1:
  preprocess  bench.c:    482 us
  lex         bench.c:     56 us
  parse       bench.c:    117 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    117 us
  link        bench_rcc_o1:  56710 us
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
| TCC       |        91 ms |
| GCC -O0   |       998 ms |
| GCC -O2   |     10130 ms |
| Clang -O0 |      1367 ms |
| Clang -O2 |     10536 ms |
