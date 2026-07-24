# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          662 |        726 |
| RCC -O1   |           79 |          624 |        703 |
| RCC -O2   |           55 |          625 |        680 |
| TCC       |           75 |          540 |        615 |
| GCC -O0   |           72 |          446 |        518 |
| GCC -O2   |          116 |          266 |        382 |
| Clang -O0 |           55 |          449 |        504 |
| Clang -O2 |           89 |          277 |        366 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1161 us
  parse       bench.c:    121 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    126 us
  link        bench_rcc:  56765 us

RCC -O1:
  preprocess  bench.c:    632 us
  parse       bench.c:    134 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    128 us
  link        bench_rcc_o1:  53929 us

RCC -O2:
  preprocess  bench.c:    610 us
  parse       bench.c:    112 us
  typecheck   bench.c:      6 us
  opt         bench.c:     19 us
  codegen     bench.c:    115 us
  link        bench_rcc_o2:  52345 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 243422 us
  parse       sqlite3.c: 107334 us
  typecheck   sqlite3.c:  19584 us
  codegen     sqlite3.c:  58618 us

RCC -O1:
  preprocess  sqlite3.c: 244682 us
  parse       sqlite3.c:  41458 us
  typecheck   sqlite3.c:  15937 us
  opt         sqlite3.c:  20578 us
  codegen     sqlite3.c:  47844 us

RCC -O2:
  preprocess  sqlite3.c: 218250 us
  parse       sqlite3.c:  39206 us
  typecheck   sqlite3.c:  11441 us
  opt         sqlite3.c: 125853 us
  codegen     sqlite3.c:  54975 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       489 ms |
| RCC -O1   |       447 ms |
| RCC -O2   |       497 ms |
| TCC       |        77 ms |
| GCC -O0   |       987 ms |
| GCC -O2   |      9737 ms |
| Clang -O0 |      1011 ms |
| Clang -O2 |     10641 ms |
