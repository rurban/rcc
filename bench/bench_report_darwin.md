# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          678 |        737 |
| RCC -O1   |           56 |          625 |        681 |
| RCC -O2   |           56 |          637 |        693 |
| TCC       |           43 |          587 |        630 |
| GCC -O0   |           89 |          485 |        574 |
| GCC -O2   |          134 |          294 |        428 |
| Clang -O0 |           62 |          492 |        554 |
| Clang -O2 |           89 |          292 |        381 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    642 us
  parse       bench.c:    133 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    120 us
  link        bench_rcc:  48073 us

RCC -O1:
  preprocess  bench.c:    547 us
  parse       bench.c:    118 us
  typecheck   bench.c:      4 us
  opt         bench.c:     14 us
  codegen     bench.c:    110 us
  link        bench_rcc_o1:  46654 us

RCC -O2:
  preprocess  bench.c:    559 us
  parse       bench.c:    122 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    113 us
  link        bench_rcc_o2:  46502 us
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
| TCC       |       103 ms |
| GCC -O0   |      1063 ms |
| GCC -O2   |     10952 ms |
| Clang -O0 |      1110 ms |
| Clang -O2 |     10714 ms |
