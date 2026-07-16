# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           68 |          787 |        855 |
| RCC -O1   |           69 |          681 |        750 |
| TCC       |           38 |          554 |        592 |
| GCC -O0   |           67 |          467 |        534 |
| GCC -O2   |          103 |          283 |        386 |
| Clang -O0 |           63 |          465 |        528 |
| Clang -O2 |           90 |          281 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    549 us
  lex         bench.c:     80 us
  parse       bench.c:    111 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    136 us
  link        bench_rcc:  51982 us

RCC -O1:
  preprocess  bench.c:    554 us
  lex         bench.c:     73 us
  parse       bench.c:    108 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    179 us
  link        bench_rcc_o1:  61838 us
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
| TCC       |       100 ms |
| GCC -O0   |      1246 ms |
| GCC -O2   |     11174 ms |
| Clang -O0 |      1263 ms |
| Clang -O2 |      9792 ms |
