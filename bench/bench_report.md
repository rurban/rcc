# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           58 |          585 |        643 |
| RCC -O1   |           43 |          614 |        657 |
| RCC -O2   |           46 |          602 |        648 |
| TCC       |           10 |          562 |        572 |
| SLIMCC    |           51 |          632 |        683 |
| XCC       |           11 |          353 |        364 |
| KEFIR     |          195 |          671 |        866 |
| KEFIR -O1 |          203 |          489 |        692 |
| CCC       |           37 |          553 |        590 |
| GCC -O0   |           73 |          570 |        643 |
| GCC -O2   |          176 |          217 |        393 |
| Clang -O0 |           89 |          623 |        712 |
| Clang -O2 |          135 |          232 |        367 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   5952 us
  parse       bench.c:    669 us
  typecheck   bench.c:     11 us
  codegen     bench.c:    635 us
  link        bench_rcc:  31922 us

RCC -O1:
  preprocess  bench.c:   6157 us
  parse       bench.c:    618 us
  typecheck   bench.c:     10 us
  opt         bench.c:     40 us
  codegen     bench.c:    619 us
  link        bench_rcc_o1:  38750 us

RCC -O2:
  preprocess  bench.c:   4030 us
  parse       bench.c:    520 us
  typecheck   bench.c:      8 us
  opt         bench.c:     31 us
  codegen     bench.c:    402 us
  link        bench_rcc_o2:  33889 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 251501 us
  parse       sqlite3.c: 153640 us
  typecheck   sqlite3.c:  13930 us
  codegen     sqlite3.c: 115886 us

RCC -O1:
  preprocess  sqlite3.c: 251685 us
  parse       sqlite3.c: 149371 us
  typecheck   sqlite3.c:  14108 us
  opt         sqlite3.c:  41662 us
  codegen     sqlite3.c: 116210 us

RCC -O2:
  preprocess  sqlite3.c: 248263 us
  parse       sqlite3.c: 156405 us
  typecheck   sqlite3.c:  13914 us
  opt         sqlite3.c: 195395 us
  codegen     sqlite3.c: 122145 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       594 ms |
| RCC -O1   |       631 ms |
| RCC -O2   |       789 ms |
| TCC       |       131 ms |
| SLIMCC    |      1260 ms |
| KEFIR     |     23276 ms |
| KEFIR -O1 |     25793 ms |
| CCC       |     17563 ms |
| GCC -O0   |     10165 ms |
| GCC -O2   |     65906 ms |
| Clang -O0 |      2905 ms |
| Clang -O2 |     37812 ms |
