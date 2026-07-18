# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           77 |          681 |        758 |
| RCC -O1   |           92 |          603 |        695 |
| RCC -O2   |           57 |          610 |        667 |
| TCC       |           84 |          591 |        675 |
| GCC -O0   |          107 |          463 |        570 |
| GCC -O2   |          104 |          276 |        380 |
| Clang -O0 |           62 |          447 |        509 |
| Clang -O2 |           86 |          277 |        363 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    908 us
  parse       bench.c:    132 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    162 us
  link        bench_rcc:  85627 us

RCC -O1:
  preprocess  bench.c:    617 us
  parse       bench.c:    113 us
  typecheck   bench.c:      5 us
  opt         bench.c:     15 us
  codegen     bench.c:    123 us
  link        bench_rcc_o1:  61754 us
```

RCC -O2:
preprocess bench.c: 837 us
parse bench.c: 137 us
typecheck bench.c: 27 us
opt bench.c: 83 us
codegen bench.c: 227 us
link bench_rcc_o2: 83382 us

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
| TCC       |        89 ms |
| GCC -O0   |       960 ms |
| GCC -O2   |      9829 ms |
| Clang -O0 |       930 ms |
| Clang -O2 |     10023 ms |
