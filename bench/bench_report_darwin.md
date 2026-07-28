# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          120 |          717 |        837 |
| RCC -O1   |           92 |          796 |        888 |
| RCC -O2   |           71 |          803 |        874 |
| TCC       |          299 |          722 |       1021 |
| GCC -O0   |          193 |          642 |        835 |
| GCC -O2   |          202 |          355 |        557 |
| Clang -O0 |          100 |          617 |        717 |
| Clang -O2 |          185 |          356 |        541 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1774 us
  parse       bench.c:    264 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    150 us
  link        bench_rcc:  72503 us

RCC -O1:
  preprocess  bench.c:    642 us
  parse       bench.c:    132 us
  typecheck   bench.c:      6 us
  opt         bench.c:     20 us
  codegen     bench.c:    114 us
  link        bench_rcc_o1:  72430 us

RCC -O2:
  preprocess  bench.c:    595 us
  parse       bench.c:    141 us
  typecheck   bench.c:      6 us
  opt         bench.c:     21 us
  codegen     bench.c:    126 us
  link        bench_rcc_o2:  68050 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 525816 us
  parse       sqlite3.c: 257100 us
  typecheck   sqlite3.c:  31920 us
  codegen     sqlite3.c:  84982 us

RCC -O1:
  preprocess  sqlite3.c: 723742 us
  parse       sqlite3.c: 179402 us
  typecheck   sqlite3.c:  36813 us
  opt         sqlite3.c:  36337 us
  codegen     sqlite3.c:  88154 us

RCC -O2:
  preprocess  sqlite3.c: 376529 us
  parse       sqlite3.c:  69683 us
  typecheck   sqlite3.c:  19386 us
  opt         sqlite3.c: 272060 us
  codegen     sqlite3.c: 113426 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1394 ms |
| RCC -O1   |       867 ms |
| RCC -O2   |       943 ms |
| TCC       |       126 ms |
| GCC -O0   |      1501 ms |
| GCC -O2   |     14680 ms |
| Clang -O0 |      1494 ms |
| Clang -O2 |     14815 ms |
