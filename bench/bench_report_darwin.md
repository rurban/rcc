# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          110 |          861 |        971 |
| RCC -O1   |          145 |          829 |        974 |
| RCC -O2   |          140 |          679 |        819 |
| TCC       |           68 |          534 |        602 |
| GCC -O0   |          169 |          503 |        672 |
| GCC -O2   |          181 |          308 |        489 |
| Clang -O0 |           76 |          507 |        583 |
| Clang -O2 |          121 |          297 |        418 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    750 us
  parse       bench.c:    139 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    123 us
  link bench_rcc:    296 us
  link        bench_rcc:  63329 us

RCC -O1:
  preprocess  bench.c:    647 us
  parse       bench.c:    132 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    134 us
  link bench_rcc_o1:    422 us
  link        bench_rcc_o1:  72041 us

RCC -O2:
  preprocess  bench.c:    592 us
  parse       bench.c:    121 us
  typecheck   bench.c:      6 us
  opt         bench.c:     20 us
  codegen     bench.c:    108 us
  link bench_rcc_o2:    195 us
  link        bench_rcc_o2:  65675 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 545739 us
  parse       sqlite3.c: 238498 us
  typecheck   sqlite3.c:  24171 us
  codegen     sqlite3.c:  97756 us
  link libsqlite3.so:   5679 us
  link        libsqlite3.so: 180381 us

RCC -O1:
  preprocess  sqlite3.c: 479600 us
  parse       sqlite3.c:  70134 us
  typecheck   sqlite3.c:  30454 us
  opt         sqlite3.c:  28376 us
  codegen     sqlite3.c: 128070 us
  link libsqlite3.so:   3809 us
  link        libsqlite3.so: 110535 us

RCC -O2:
  preprocess  sqlite3.c: 464671 us
  parse       sqlite3.c:  98021 us
  typecheck   sqlite3.c:  25929 us
  opt         sqlite3.c: 347226 us
  codegen     sqlite3.c: 119184 us
  link libsqlite3.so:   3146 us
  link        libsqlite3.so: 113613 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1365 ms |
| RCC -O1   |       716 ms |
| RCC -O2   |       848 ms |
| TCC       |       131 ms |
| GCC -O0   |      1206 ms |
| GCC -O2   |     11851 ms |
| Clang -O0 |      1274 ms |
| Clang -O2 |     10560 ms |
