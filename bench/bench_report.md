# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           13 |          767 |        780 |
| RCC -O1   |           12 |          769 |        781 |
| RCC -O2   |           17 |          784 |        801 |
| TCC       |           20 |          751 |        771 |
| SLIMCC    |           58 |          839 |        897 |
| XCC       |           18 |          475 |        493 |
| KEFIR     |          260 |          909 |       1169 |
| KEFIR -O1 |          253 |          650 |        903 |
| CCC       |           62 |          766 |        828 |
| GCC -O0   |           85 |          756 |        841 |
| GCC -O2   |          243 |          292 |        535 |
| Clang -O0 |          118 |          832 |        950 |
| Clang -O2 |          201 |          311 |        512 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   6841 us
  parse       bench.c:    764 us
  typecheck   bench.c:     20 us
  codegen     bench.c:    734 us
  link        bench_rcc:  576 us

RCC -O1:
  preprocess  bench.c:   5248 us
  parse       bench.c:    606 us
  typecheck   bench.c:     10 us
  opt         bench.c:     37 us
  codegen     bench.c:    467 us
  link        bench_o1:   452 us

RCC -O2:
  preprocess  bench.c:   8297 us
  parse       bench.c:    650 us
  typecheck   bench.c:     10 us
  opt         bench.c:     41 us
  codegen     bench.c:    492 us
  link        bench_o2:   483 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 329262 us
  parse       sqlite3.c: 214777 us
  typecheck   sqlite3.c:  15497 us
  codegen     sqlite3.c: 236611 us
  link    libsqlite3.so:   8461 us

RCC -O1:
  preprocess  sqlite3.c: 324186 us
  parse       sqlite3.c: 215337 us
  typecheck   sqlite3.c:  15497 us
  opt         sqlite3.c:  50898 us
  codegen     sqlite3.c: 230919 us
  link    libsqlite3.so:  11978 us

RCC -O2:
  preprocess  sqlite3.c: 339931 us
  parse       sqlite3.c: 208685 us
  typecheck   sqlite3.c:  15519 us
  opt         sqlite3.c: 260161 us
  codegen     sqlite3.c: 236178 us
  link    libsqlite3.so:   8799 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1255 ms |
| RCC -O1   |      1330 ms |
| RCC -O2   |      1538 ms |
| TCC       |       172 ms |
| SLIMCC    |      1831 ms |
| KEFIR     |     30941 ms |
| KEFIR -O1 |     33858 ms |
| CCC       |     20925 ms |
| GCC -O0   |      6884 ms |
| GCC -O2   |     41067 ms |
| Clang -O0 |      1889 ms |
| Clang -O2 |     23689 ms |
