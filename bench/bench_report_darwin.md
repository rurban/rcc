# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          118 |          879 |        997 |
| RCC -O1   |          141 |          835 |        976 |
| RCC -O2   |           91 |          839 |        930 |
| TCC       |           92 |          771 |        863 |
| GCC -O0   |          153 |          761 |        914 |
| GCC -O2   |          445 |          358 |        803 |
| Clang -O0 |          114 |          685 |        799 |
| Clang -O2 |          179 |          384 |        563 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    925 us
  parse       bench.c:    154 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    160 us
  link bench_rcc:    640 us
  link        bench_rcc:  70373 us

RCC -O1:
  preprocess  bench.c:    861 us
  parse       bench.c:    135 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    694 us
  link bench_rcc_o1:    367 us
  link        bench_rcc_o1: 111830 us

RCC -O2:
  preprocess  bench.c:    677 us
  parse       bench.c:    164 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    138 us
  link bench_rcc_o2:    111 us
  link        bench_rcc_o2:  88376 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 493014 us
  parse       sqlite3.c: 193927 us
  typecheck   sqlite3.c:  30309 us
  codegen     sqlite3.c: 152628 us
  link libsqlite3.so:  21851 us

RCC -O1:
  preprocess  sqlite3.c: 364762 us
  parse       sqlite3.c:  87980 us
  typecheck   sqlite3.c:  22931 us
  opt         sqlite3.c:  29784 us
  codegen     sqlite3.c: 163354 us
  link libsqlite3.so:  69072 us

RCC -O2:
  preprocess  sqlite3.c: 337994 us
  parse       sqlite3.c:  68972 us
  typecheck   sqlite3.c:  16716 us
  opt         sqlite3.c: 309347 us
  codegen     sqlite3.c: 185748 us
  link libsqlite3.so:  23095 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1361 ms |
| RCC -O1   |      1073 ms |
| RCC -O2   |      1414 ms |
| TCC       |       217 ms |
| GCC -O0   |      2208 ms |
| GCC -O2   |     17810 ms |
| Clang -O0 |      1878 ms |
| Clang -O2 |     19227 ms |
