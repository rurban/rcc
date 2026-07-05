# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          596 |        659 |
| RCC -O1   |           57 |          671 |        728 |
| TCC       |           38 |          551 |        589 |
| GCC -O0   |           84 |          449 |        533 |
| GCC -O2   |          112 |          276 |        388 |
| Clang -O0 |           61 |          465 |        526 |
| Clang -O2 |          103 |          274 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    593 us
  lex         bench.c:     84 us
  parse       bench.c:    126 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    163 us
  link        bench_rcc:  72364 us

RCC -O1:
  preprocess  bench.c:    654 us
  lex         bench.c:    147 us
  parse       bench.c:    122 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    155 us
  link        bench_rcc_o1:  54987 us
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
| GCC -O0   |      1003 ms |
| GCC -O2   |     10985 ms |
| Clang -O0 |      1048 ms |
| Clang -O2 |      9528 ms |
