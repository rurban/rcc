# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           46 |          583 |        629 |
| RCC -O1   |           46 |          584 |        630 |
| RCC -O2   |           50 |          581 |        631 |
| TCC       |           39 |          511 |        550 |
| GCC -O0   |           56 |          434 |        490 |
| GCC -O2   |           87 |          271 |        358 |
| Clang -O0 |           51 |          433 |        484 |
| Clang -O2 |           85 |          262 |        347 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    539 us
  parse       bench.c:    104 us
  typecheck   bench.c:      5 us
  codegen     bench.c:     97 us
  link bench_rcc:    108 us
  link        bench_rcc:  49842 us

RCC -O1:
  preprocess  bench.c:    501 us
  parse       bench.c:    106 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    100 us
  link bench_rcc_o1:    120 us
  link        bench_rcc_o1:  40905 us

RCC -O2:
  preprocess  bench.c:    491 us
  parse       bench.c:    105 us
  typecheck   bench.c:      4 us
  opt         bench.c:     18 us
  codegen     bench.c:    100 us
  link bench_rcc_o2:    168 us
  link        bench_rcc_o2:  40494 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 183069 us
  parse       sqlite3.c:  40648 us
  typecheck   sqlite3.c:  11386 us
  codegen     sqlite3.c:  80726 us
  link libsqlite3.so:  13379 us

RCC -O1:
  preprocess  sqlite3.c: 180178 us
  parse       sqlite3.c:  40029 us
  typecheck   sqlite3.c:  11272 us
  opt         sqlite3.c:  18227 us
  codegen     sqlite3.c:  80673 us
  link libsqlite3.so:  13887 us

RCC -O2:
  preprocess  sqlite3.c: 179801 us
  parse       sqlite3.c:  40252 us
  typecheck   sqlite3.c:  11246 us
  opt         sqlite3.c: 119702 us
  codegen     sqlite3.c:  84073 us
  link libsqlite3.so:  14252 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       504 ms |
| RCC -O1   |       527 ms |
| RCC -O2   |       649 ms |
| TCC       |        88 ms |
| GCC -O0   |       929 ms |
| GCC -O2   |      8895 ms |
| Clang -O0 |       938 ms |
| Clang -O2 |      8751 ms |
