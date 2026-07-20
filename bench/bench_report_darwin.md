# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          381 |          699 |       1080 |
| RCC -O1   |           77 |          628 |        705 |
| RCC -O2   |           89 |          630 |        719 |
| TCC       |           49 |          568 |        617 |
| GCC -O0   |           89 |          462 |        551 |
| GCC -O2   |          152 |          283 |        435 |
| Clang -O0 |          110 |          533 |        643 |
| Clang -O2 |          129 |          313 |        442 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    941 us
  parse       bench.c:    165 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    611 us
  link        bench_rcc:  91872 us

RCC -O1:
  preprocess  bench.c:    732 us
  parse       bench.c:    268 us
  typecheck   bench.c:     10 us
  opt         bench.c:     41 us
  codegen     bench.c:    174 us
  link        bench_rcc_o1:  69134 us
```

RCC -O2:
preprocess bench.c: 615 us
parse bench.c: 111 us
typecheck bench.c: 4 us
opt bench.c: 16 us
codegen bench.c: 154 us
link bench_rcc_o2: 69937 us

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
| TCC       |       439 ms |
| GCC -O0   |      2078 ms |
| GCC -O2   |     13936 ms |
| Clang -O0 |      1100 ms |
| Clang -O2 |     10432 ms |
