# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          625 |        674 |
| RCC -O1   |           48 |          581 |        629 |
| RCC -O2   |           49 |          580 |        629 |
| TCC       |           35 |          512 |        547 |
| GCC -O0   |           58 |          433 |        491 |
| GCC -O2   |           89 |          262 |        351 |
| Clang -O0 |           53 |          433 |        486 |
| Clang -O2 |           81 |          263 |        344 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    561 us
  parse       bench.c:    105 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    101 us
  link        bench_rcc:  42985 us

RCC -O1:
  preprocess  bench.c:    597 us
  parse       bench.c:    110 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    107 us
  link        bench_rcc_o1:  45326 us

RCC -O2:
  preprocess  bench.c:    488 us
  parse       bench.c:    102 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:     99 us
  link        bench_rcc_o2:  43448 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 213410 us
  parse       sqlite3.c:  40196 us
  typecheck   sqlite3.c:  11652 us
  codegen     sqlite3.c:  40568 us

RCC -O1:
  preprocess  sqlite3.c: 215498 us
  parse       sqlite3.c:  44798 us
  typecheck   sqlite3.c:  12496 us
  opt         sqlite3.c:  21196 us
  codegen     sqlite3.c:  46931 us

RCC -O2:
  preprocess  sqlite3.c: 204056 us
  parse       sqlite3.c:  42730 us
  typecheck   sqlite3.c:  11471 us
  opt         sqlite3.c: 122208 us
  codegen     sqlite3.c:  42814 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       463 ms |
| RCC -O1   |       478 ms |
| RCC -O2   |       577 ms |
| TCC       |        63 ms |
| GCC -O0   |       888 ms |
| GCC -O2   |      8738 ms |
| Clang -O0 |       916 ms |
| Clang -O2 |      8899 ms |
