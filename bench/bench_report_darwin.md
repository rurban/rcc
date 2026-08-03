# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          181 |          874 |       1055 |
| RCC -O1   |          116 |          875 |        991 |
| RCC -O2   |          193 |          868 |       1061 |
| TCC       |          114 |          757 |        871 |
| GCC -O0   |          171 |          665 |        836 |
| GCC -O2   |          144 |          368 |        512 |
| Clang -O0 |          132 |          669 |        801 |
| Clang -O2 |          165 |          416 |        581 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1470 us
  parse       bench.c:    150 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    146 us
  link bench_rcc:    965 us
  link        bench_rcc:  70129 us

RCC -O1:
  preprocess  bench.c:    695 us
  parse       bench.c:    130 us
  typecheck   bench.c:      5 us
  opt         bench.c:     59 us
  codegen     bench.c:    125 us
  link bench_rcc_o1:    645 us
  link        bench_rcc_o1:  75183 us

RCC -O2:
  preprocess  bench.c:    718 us
  parse       bench.c:    135 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    114 us
  link bench_rcc_o2:    808 us
  link        bench_rcc_o2:  73253 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 547663 us
  parse       sqlite3.c: 246445 us
  typecheck   sqlite3.c:  40979 us
  codegen     sqlite3.c: 247768 us
  link libsqlite3.so:  24342 us

RCC -O1:
  preprocess  sqlite3.c: 481107 us
  parse       sqlite3.c:  54252 us
  typecheck   sqlite3.c:  14659 us
  opt         sqlite3.c:  29774 us
  codegen     sqlite3.c: 224695 us
  link libsqlite3.so:  23199 us

RCC -O2:
  preprocess  sqlite3.c: 494472 us
  parse       sqlite3.c: 104885 us
  typecheck   sqlite3.c:  21231 us
  opt         sqlite3.c: 387227 us
  codegen     sqlite3.c: 220442 us
  link libsqlite3.so:  24906 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2151 ms |
| RCC -O1   |      1255 ms |
| RCC -O2   |      1812 ms |
| TCC       |       279 ms |
| GCC -O0   |      2028 ms |
| GCC -O2   |     18376 ms |
| Clang -O0 |      1905 ms |
| Clang -O2 |     19494 ms |
