# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          642 |        729 |
| RCC -O1   |           86 |          723 |        809 |
| RCC -O2   |          107 |          681 |        788 |
| TCC       |           68 |          650 |        718 |
| GCC -O0   |           92 |          501 |        593 |
| GCC -O2   |          168 |          315 |        483 |
| Clang -O0 |           96 |          569 |        665 |
| Clang -O2 |          124 |          319 |        443 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1486 us
  parse       bench.c:    554 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    478 us
  link        bench_rcc:  65363 us

RCC -O1:
  preprocess  bench.c:    740 us
  parse       bench.c:    193 us
  typecheck   bench.c:      6 us
  opt         bench.c:     29 us
  codegen     bench.c:    177 us
  link        bench_rcc_o1:  66043 us

RCC -O2:
  preprocess  bench.c:   1749 us
  parse       bench.c:    413 us
  typecheck   bench.c:      6 us
  opt         bench.c:     26 us
  codegen     bench.c:    536 us
  link        bench_rcc_o2:  77510 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 392855 us
  parse       sqlite3.c:  70132 us
  typecheck   sqlite3.c:  13002 us
  codegen     sqlite3.c:  53883 us

RCC -O1:
  preprocess  sqlite3.c: 259761 us
  parse       sqlite3.c:  50540 us
  typecheck   sqlite3.c:  12524 us
  opt         sqlite3.c:  20141 us
  codegen     sqlite3.c:  56915 us

RCC -O2:
  preprocess  sqlite3.c: 249359 us
  parse       sqlite3.c:  53715 us
  typecheck   sqlite3.c:  16747 us
  opt         sqlite3.c: 154375 us
  codegen     sqlite3.c:  59328 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       652 ms |
| RCC -O1   |       664 ms |
| RCC -O2   |       895 ms |
| TCC       |        92 ms |
| GCC -O0   |      1326 ms |
| GCC -O2   |     14535 ms |
| Clang -O0 |      1107 ms |
| Clang -O2 |     12330 ms |
