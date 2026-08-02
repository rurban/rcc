# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          172 |          723 |        895 |
| RCC -O1   |           96 |          780 |        876 |
| RCC -O2   |           76 |          758 |        834 |
| TCC       |           70 |          623 |        693 |
| GCC -O0   |           85 |          490 |        575 |
| GCC -O2   |          133 |          313 |        446 |
| Clang -O0 |           70 |          604 |        674 |
| Clang -O2 |          135 |          361 |        496 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    880 us
  parse       bench.c:    189 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    152 us
  link bench_rcc:    183 us
  link        bench_rcc:  76432 us

RCC -O1:
  preprocess  bench.c:    618 us
  parse       bench.c:    132 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    107 us
  link bench_rcc_o1:    188 us
  link        bench_rcc_o1:  54601 us

RCC -O2:
  preprocess  bench.c:    645 us
  parse       bench.c:    144 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    134 us
  link bench_rcc_o2:    142 us
  link        bench_rcc_o2:  66600 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 318076 us
  parse       sqlite3.c: 153916 us
  typecheck   sqlite3.c:  32535 us
  codegen     sqlite3.c: 133273 us
  link libsqlite3.so:  20506 us

RCC -O1:
  preprocess  sqlite3.c: 389689 us
  parse       sqlite3.c:  78982 us
  typecheck   sqlite3.c:  25046 us
  opt         sqlite3.c:  32228 us
  codegen     sqlite3.c: 167064 us
  link libsqlite3.so:  21595 us

RCC -O2:
  preprocess  sqlite3.c: 323635 us
  parse       sqlite3.c:  57050 us
  typecheck   sqlite3.c:  19882 us
  opt         sqlite3.c: 215009 us
  codegen     sqlite3.c: 169226 us
  link libsqlite3.so:  34588 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       986 ms |
| RCC -O1   |       863 ms |
| RCC -O2   |      1067 ms |
| TCC       |       137 ms |
| GCC -O0   |      1424 ms |
| GCC -O2   |     18835 ms |
| Clang -O0 |      2291 ms |
| Clang -O2 |     16306 ms |
