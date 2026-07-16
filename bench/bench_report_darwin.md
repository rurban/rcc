# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          592 |        647 |
| RCC -O1   |           54 |          649 |        703 |
| TCC       |           41 |          559 |        600 |
| GCC -O0   |           74 |          449 |        523 |
| GCC -O2   |          102 |          272 |        374 |
| Clang -O0 |           59 |          448 |        507 |
| Clang -O2 |           85 |          281 |        366 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    536 us
  lex         bench.c:     71 us
  parse       bench.c:    101 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    122 us
  link        bench_rcc:  48644 us

RCC -O1:
  preprocess  bench.c:    511 us
  lex         bench.c:     72 us
  parse       bench.c:    111 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    127 us
  link        bench_rcc_o1:  49825 us
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
| GCC -O0   |       960 ms |
| GCC -O2   |      9721 ms |
| Clang -O0 |       985 ms |
| Clang -O2 |      9642 ms |
