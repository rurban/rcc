# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          628 |        682 |
| RCC -O1   |           53 |          649 |        702 |
| TCC       |           44 |          529 |        573 |
| GCC -O0   |           79 |          468 |        547 |
| GCC -O2   |          102 |          272 |        374 |
| Clang -O0 |           60 |          451 |        511 |
| Clang -O2 |           95 |          277 |        372 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    834 us
  lex         bench.c:     74 us
  parse       bench.c:     99 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    141 us
  link        bench_rcc:  46534 us

RCC -O1:
  preprocess  bench.c:    430 us
  lex         bench.c:     71 us
  parse       bench.c:     96 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    136 us
  link        bench_rcc_o1:  46255 us
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
| TCC       |       111 ms |
| GCC -O0   |       979 ms |
| GCC -O2   |     10838 ms |
| Clang -O0 |       997 ms |
| Clang -O2 |     10123 ms |
