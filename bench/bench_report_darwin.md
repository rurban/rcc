# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          648 |        703 |
| RCC -O1   |           53 |          641 |        694 |
| RCC -O2   |           58 |          644 |        702 |
| TCC       |           41 |          575 |        616 |
| GCC -O0   |           88 |          505 |        593 |
| GCC -O2   |          118 |          274 |        392 |
| Clang -O0 |           52 |          454 |        506 |
| Clang -O2 |           82 |          273 |        355 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    134 us
  link        bench_rcc     :    112 us
  link        bench_rcc     :  46784 us

RCC -O1:
  preprocess  bench.c       :    582 us
  parse       bench.c       :    111 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    113 us
  link        bench_o1      :  57379 us

RCC -O2:
  preprocess  bench.c       :    635 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    122 us
  link        bench_o2      :    125 us
  link        bench_o2      :  46474 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 233538 us
  parse       sqlite3.c     :  49540 us
  typecheck   sqlite3.c     :  14435 us
  codegen     sqlite3.c     :  96296 us
  link        sqlite3.so    :  13745 us

RCC -O1:
  preprocess  sqlite3.c     : 221173 us
  parse       sqlite3.c     :  48720 us
  typecheck   sqlite3.c     :  12095 us
  opt         sqlite3.c     :  19341 us
  codegen     sqlite3.c     :  95786 us
  link        sqlite3.so    :  14262 us

RCC -O2:
  preprocess  sqlite3.c     : 218425 us
  parse       sqlite3.c     :  48152 us
  typecheck   sqlite3.c     :  13962 us
  opt         sqlite3.c     : 154925 us
  codegen     sqlite3.c     : 101335 us
  link        sqlite3.so    :  15499 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       775 ms |
| RCC -O1   |       572 ms |
| RCC -O2   |       660 ms |
| TCC       |        97 ms |
| GCC -O0   |      1056 ms |
| GCC -O2   |      9923 ms |
| Clang -O0 |      1152 ms |
| Clang -O2 |     10261 ms |
