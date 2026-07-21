# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          670 |        749 |
| RCC -O1   |           89 |          620 |        709 |
| RCC -O2   |           65 |          636 |        701 |
| TCC       |           66 |          547 |        613 |
| GCC -O0   |          118 |          437 |        555 |
| GCC -O2   |          128 |          272 |        400 |
| Clang -O0 |           58 |          436 |        494 |
| Clang -O2 |           87 |          263 |        350 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2983 us
  parse       bench.c:    356 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    248 us
  link        bench_rcc:  91477 us

RCC -O1:
  preprocess  bench.c:    583 us
  parse       bench.c:    112 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    115 us
  link        bench_rcc_o1:  52912 us

RCC -O2:
  preprocess  bench.c:    576 us
  parse       bench.c:    116 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    112 us
  link        bench_rcc_o2:  50994 us
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
| TCC       |       111 ms |
| GCC -O0   |       901 ms |
| GCC -O2   |     10708 ms |
| Clang -O0 |      1043 ms |
| Clang -O2 |     10211 ms |
