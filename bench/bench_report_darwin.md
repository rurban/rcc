# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          136 |          709 |        845 |
| RCC -O1   |          113 |          721 |        834 |
| RCC -O2   |          113 |          749 |        862 |
| TCC       |          128 |          692 |        820 |
| GCC -O0   |          205 |          601 |        806 |
| GCC -O2   |          246 |          353 |        599 |
| Clang -O0 |          152 |          601 |        753 |
| Clang -O2 |          204 |          367 |        571 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1120 us
  parse       bench.c:    368 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    292 us
  link        bench_rcc: 109473 us

RCC -O1:
  preprocess  bench.c:    611 us
  parse       bench.c:    104 us
  typecheck   bench.c:      5 us
  opt         bench.c:     16 us
  codegen     bench.c:    111 us
  link        bench_rcc_o1:  59716 us
```

RCC -O2:
preprocess bench.c: 577 us
parse bench.c: 111 us
typecheck bench.c: 4 us
opt bench.c: 16 us
codegen bench.c: 111 us
link bench_rcc_o2: 56790 us

```

## RCC Substep Timing -- sqlite3.c

```

RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

```

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       188 ms |
| GCC -O0   |      1623 ms |
| GCC -O2   |     15246 ms |
| Clang -O0 |      1341 ms |
| Clang -O2 |     12908 ms |
