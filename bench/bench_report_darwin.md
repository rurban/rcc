# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          666 |        754 |
| RCC -O1   |           53 |          651 |        704 |
| RCC -O2   |           71 |          674 |        745 |
| TCC       |          103 |          602 |        705 |
| GCC -O0   |          140 |          630 |        770 |
| GCC -O2   |          223 |          353 |        576 |
| Clang -O0 |           78 |          538 |        616 |
| Clang -O2 |          153 |          337 |        490 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    685 us
  parse       bench.c:    147 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    170 us
  link        bench_rcc:  54022 us

RCC -O1:
  preprocess  bench.c:    536 us
  parse       bench.c:    120 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    118 us
  link        bench_rcc_o1:  47903 us

RCC -O2:
  preprocess  bench.c:    538 us
  parse       bench.c:    110 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    102 us
  link        bench_rcc_o2:  47300 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 260126 us
  parse       sqlite3.c: 414947 us
  typecheck   sqlite3.c:  27492 us
  codegen     sqlite3.c:  89707 us

RCC -O1:
  preprocess  sqlite3.c: 334208 us
  parse       sqlite3.c:  64484 us
  typecheck   sqlite3.c:  23287 us
  opt         sqlite3.c:  27741 us
  codegen     sqlite3.c:  75347 us

RCC -O2:
  preprocess  sqlite3.c: 265630 us
  parse       sqlite3.c:  48209 us
  typecheck   sqlite3.c:  11987 us
  opt         sqlite3.c: 136657 us
  codegen     sqlite3.c:  57071 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       837 ms |
| RCC -O1   |       540 ms |
| RCC -O2   |       644 ms |
| TCC       |       137 ms |
| GCC -O0   |      1564 ms |
| GCC -O2   |     14191 ms |
| Clang -O0 |      1731 ms |
| Clang -O2 |     14137 ms |
