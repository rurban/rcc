# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           50 |          652 |        702 |
| RCC -O1   |           66 |          688 |        754 |
| TCC       |           41 |          553 |        594 |
| GCC -O0   |           70 |          474 |        544 |
| GCC -O2   |          110 |          272 |        382 |
| Clang -O0 |           55 |          441 |        496 |
| Clang -O2 |           92 |          264 |        356 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    488 us
  lex         bench.c:     71 us
  parse       bench.c:     91 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    128 us
  link        bench_rcc:  43837 us

RCC -O1:
  preprocess  bench.c:    454 us
  lex         bench.c:     72 us
  parse       bench.c:     94 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    140 us
  link        bench_rcc_o1:  46286 us
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
| TCC       |        90 ms |
| GCC -O0   |       911 ms |
| GCC -O2   |     10880 ms |
| Clang -O0 |       966 ms |
| Clang -O2 |      9696 ms |
