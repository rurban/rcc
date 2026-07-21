# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          707 |        773 |
| RCC -O1   |           72 |          647 |        719 |
| RCC -O2   |           84 |          653 |        737 |
| TCC       |           73 |          581 |        654 |
| GCC -O0   |          127 |          466 |        593 |
| GCC -O2   |          136 |          285 |        421 |
| Clang -O0 |           74 |          469 |        543 |
| Clang -O2 |          115 |          284 |        399 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    837 us
  parse       bench.c:    169 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    163 us
  link        bench_rcc:  68479 us

RCC -O1:
  preprocess  bench.c:    593 us
  parse       bench.c:    129 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    126 us
  link        bench_rcc_o1:  53096 us

RCC -O2:
  preprocess  bench.c:    571 us
  parse       bench.c:    114 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    109 us
  link        bench_rcc_o2:  51981 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       112 ms |
| GCC -O0   |      1015 ms |
| GCC -O2   |     10206 ms |
| Clang -O0 |      1012 ms |
| Clang -O2 |     10193 ms |
