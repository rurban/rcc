# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          148 |          861 |       1009 |
| RCC -O1   |           79 |          872 |        951 |
| RCC -O2   |          132 |          840 |        972 |
| TCC       |           80 |          758 |        838 |
| GCC -O0   |          139 |          635 |        774 |
| GCC -O2   |          208 |          352 |        560 |
| Clang -O0 |           92 |          629 |        721 |
| Clang -O2 |          165 |          362 |        527 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1237 us
  parse       bench.c:    158 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    135 us
  link        bench_rcc:  67154 us

RCC -O1:
  preprocess  bench.c:    657 us
  parse       bench.c:    153 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    128 us
  link        bench_rcc_o1:  67509 us

RCC -O2:
  preprocess  bench.c:   3879 us
  parse       bench.c:    440 us
  typecheck   bench.c:     11 us
  opt         bench.c:     54 us
  codegen     bench.c:    321 us
  link        bench_rcc_o2:  84072 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 674762 us
  parse       sqlite3.c: 262173 us
  typecheck   sqlite3.c:  34036 us
  codegen     sqlite3.c: 108814 us

RCC -O1:
  preprocess  sqlite3.c: 503757 us
  parse       sqlite3.c: 107452 us
  typecheck   sqlite3.c:  34634 us
  opt         sqlite3.c:  47323 us
  codegen     sqlite3.c: 134113 us

RCC -O2:
  preprocess  sqlite3.c: 490045 us
  parse       sqlite3.c: 147421 us
  typecheck   sqlite3.c:  28170 us
  opt         sqlite3.c: 339743 us
  codegen     sqlite3.c: 122334 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1246 ms |
| RCC -O1   |       974 ms |
| RCC -O2   |      1201 ms |
| TCC       |       124 ms |
| GCC -O0   |      1978 ms |
| GCC -O2   |     17366 ms |
| Clang -O0 |      1915 ms |
| Clang -O2 |     19082 ms |
