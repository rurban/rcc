# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          745 |        804 |
| RCC -O1   |          167 |          703 |        870 |
| RCC -O2   |          122 |          656 |        778 |
| TCC       |           53 |          577 |        630 |
| GCC -O0   |           93 |          523 |        616 |
| GCC -O2   |          141 |          286 |        427 |
| Clang -O0 |           68 |          517 |        585 |
| Clang -O2 |          130 |          291 |        421 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    791 us
  parse       bench.c:    152 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    159 us
  link        bench_rcc:  81474 us

RCC -O1:
  preprocess  bench.c:    655 us
  parse       bench.c:    137 us
  typecheck   bench.c:      4 us
  opt         bench.c:     18 us
  codegen     bench.c:    117 us
  link        bench_rcc_o1:  68140 us

RCC -O2:
  preprocess  bench.c:    675 us
  parse       bench.c:    139 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    128 us
  link        bench_rcc_o2:  80705 us
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
| TCC       |       155 ms |
| GCC -O0   |      1112 ms |
| GCC -O2   |     12754 ms |
| Clang -O0 |      1235 ms |
| Clang -O2 |     12209 ms |
