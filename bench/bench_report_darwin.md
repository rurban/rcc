# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           94 |          752 |        846 |
| RCC -O1   |          102 |          729 |        831 |
| RCC -O2   |           79 |          732 |        811 |
| TCC       |           54 |          602 |        656 |
| GCC -O0   |           86 |          505 |        591 |
| GCC -O2   |          144 |          324 |        468 |
| Clang -O0 |          107 |          697 |        804 |
| Clang -O2 |          170 |          405 |        575 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    880 us
  parse       bench.c:    136 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    199 us
  link        bench_rcc:  65738 us

RCC -O1:
  preprocess  bench.c:    870 us
  parse       bench.c:    126 us
  typecheck   bench.c:      6 us
  opt         bench.c:     18 us
  codegen     bench.c:    123 us
  link        bench_rcc_o1:  57329 us

RCC -O2:
  preprocess  bench.c:    620 us
  parse       bench.c:    131 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    107 us
  link        bench_rcc_o2:  64942 us
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
| TCC       |       347 ms |
| GCC -O0   |      2393 ms |
| GCC -O2   |     20589 ms |
| Clang -O0 |      2243 ms |
| Clang -O2 |     18585 ms |
