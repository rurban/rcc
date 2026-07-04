# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          163 |          790 |        953 |
| RCC -O1   |           82 |          709 |        791 |
| TCC       |           86 |          746 |        832 |
| GCC -O0   |          189 |          610 |        799 |
| GCC -O2   |          217 |          329 |        546 |
| Clang -O0 |          111 |          664 |        775 |
| Clang -O2 |          255 |          342 |        597 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1269 us
  lex         bench.c:     79 us
  parse       bench.c:     87 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    154 us
  link        bench_rcc:  56236 us

RCC -O1:
  preprocess  bench.c:   1636 us
  lex         bench.c:     78 us
  parse       bench.c:     80 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     25 us
  codegen     bench.c:    151 us
  link        bench_rcc_o1:  65867 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 2669347 us
  lex         sqlite3.c: 109889 us
  parse       sqlite3.c: 118776 us
  typecheck   sqlite3.c:  20490 us
  codegen     sqlite3.c: 3890983 us

RCC -O1:
  preprocess  sqlite3.c: 1977250 us
  lex         sqlite3.c: 101686 us
  parse       sqlite3.c:  88157 us
  typecheck   sqlite3.c:  19631 us
  opt(CTFE)   sqlite3.c:  21569 us
  codegen     sqlite3.c: 3315803 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      6349 ms |
| RCC -O1   |      5668 ms |
| TCC       |       133 ms |
| GCC -O0   |      1260 ms |
| GCC -O2   |     20052 ms |
| Clang -O0 |      1754 ms |
| Clang -O2 |     16826 ms |
