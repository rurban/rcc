# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           96 |          782 |        878 |
| RCC -O1   |           84 |          688 |        772 |
| TCC       |           67 |          653 |        720 |
| GCC -O0   |          147 |          554 |        701 |
| GCC -O2   |          245 |          322 |        567 |
| Clang -O0 |          137 |          563 |        700 |
| Clang -O2 |          203 |          330 |        533 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    697 us
  lex         bench.c:     99 us
  parse       bench.c:    117 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    159 us
  link        bench_rcc:  74190 us

RCC -O1:
  preprocess  bench.c:    588 us
  lex         bench.c:     81 us
  parse       bench.c:    102 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    154 us
  link        bench_rcc_o1:  76542 us
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
| TCC       |       182 ms |
| GCC -O0   |      1778 ms |
| GCC -O2   |     20886 ms |
| Clang -O0 |      1730 ms |
| Clang -O2 |     17196 ms |
