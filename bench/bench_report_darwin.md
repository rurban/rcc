# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          768 |        877 |
| RCC -O1   |          107 |          708 |        815 |
| RCC -O2   |          120 |          713 |        833 |
| TCC       |           80 |          606 |        686 |
| GCC -O0   |          155 |          660 |        815 |
| GCC -O2   |          236 |          319 |        555 |
| Clang -O0 |          102 |          618 |        720 |
| Clang -O2 |          161 |          413 |        574 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1644 us
  parse       bench.c:    187 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    517 us
  link        bench_rcc:  83804 us

RCC -O1:
  preprocess  bench.c:   1678 us
  parse       bench.c:    404 us
  typecheck   bench.c:     10 us
  opt         bench.c:     49 us
  codegen     bench.c:    352 us
  link        bench_rcc_o1:  91970 us

RCC -O2:
  preprocess  bench.c:   1025 us
  parse       bench.c:    448 us
  typecheck   bench.c:     11 us
  opt         bench.c:     30 us
  codegen     bench.c:    480 us
  link        bench_rcc_o2:  94641 us
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
| TCC       |       203 ms |
| GCC -O0   |      2376 ms |
| GCC -O2   |     17653 ms |
| Clang -O0 |      2107 ms |
| Clang -O2 |     16010 ms |
