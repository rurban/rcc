# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           91 |          634 |        725 |
| RCC -O1   |           76 |          700 |        776 |
| TCC       |           42 |          573 |        615 |
| GCC -O0   |           97 |          482 |        579 |
| GCC -O2   |          160 |          301 |        461 |
| Clang -O0 |           79 |          479 |        558 |
| Clang -O2 |          121 |          286 |        407 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    602 us
  lex         bench.c:     95 us
  parse       bench.c:    223 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    181 us
  link        bench_rcc:  63613 us

RCC -O1:
  preprocess  bench.c:    610 us
  lex         bench.c:     79 us
  parse       bench.c:    166 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     20 us
  codegen     bench.c:    156 us
  link        bench_rcc_o1:  69595 us
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
| TCC       |       106 ms |
| GCC -O0   |      1421 ms |
| GCC -O2   |     13254 ms |
| Clang -O0 |      1248 ms |
| Clang -O2 |     12846 ms |
