# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          127 |          862 |        989 |
| RCC -O1   |          143 |          834 |        977 |
| RCC -O2   |          170 |          853 |       1023 |
| TCC       |           76 |          893 |        969 |
| GCC -O0   |          167 |          669 |        836 |
| GCC -O2   |          184 |          381 |        565 |
| Clang -O0 |          123 |          642 |        765 |
| Clang -O2 |          145 |          362 |        507 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1600 us
  parse       bench.c:    259 us
  typecheck   bench.c:     14 us
  codegen     bench.c:    306 us
  link        bench_rcc:  82459 us

RCC -O1:
  preprocess  bench.c:    633 us
  parse       bench.c:    141 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    135 us
  link        bench_rcc_o1:  85385 us

RCC -O2:
  preprocess  bench.c:   1635 us
  parse       bench.c:    149 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    120 us
  link        bench_rcc_o2:  81101 us
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
| TCC       |       222 ms |
| GCC -O0   |      2069 ms |
| GCC -O2   |     19707 ms |
| Clang -O0 |      1919 ms |
| Clang -O2 |     19310 ms |
