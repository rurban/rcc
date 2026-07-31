# Linux RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           19 |          583 |        602 |
| RCC -O1   |           17 |          576 |        593 |
| RCC -O2   |           13 |          560 |        573 |
| TCC       |           14 |          580 |        594 |
| SLIMCC    |           49 |          635 |        684 |
| XCC       |           14 |          355 |        369 |
| KEFIR     |          236 |          695 |        931 |
| KEFIR -O1 |          194 |          521 |        715 |
| CCC       |           37 |          584 |        621 |
| GCC -O0   |           75 |          581 |        656 |
| GCC -O2   |          202 |          215 |        417 |
| Clang -O0 |          106 |          687 |        793 |
| Clang -O2 |          177 |          246 |        423 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   8433 us
  parse       bench.c:    832 us
  typecheck   bench.c:     24 us
  codegen     bench.c:    950 us
  link        bench_rcc:  581 us

RCC -O1:
  preprocess  bench.c:   7916 us
  parse       bench.c:    721 us
  typecheck   bench.c:     87 us
  opt         bench.c:     45 us
  codegen     bench.c:   1247 us
  link        bench_o1:   294 us

RCC -O2:
  preprocess  bench.c:   4335 us
  parse       bench.c:    419 us
  typecheck   bench.c:      6 us
  opt         bench.c:     30 us
  codegen     bench.c:    449 us
  link bench_rcc_o2:    335 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 276118 us
  parse       sqlite3.c: 171507 us
  typecheck   sqlite3.c:  15441 us
  codegen     sqlite3.c: 135867 us

RCC -O1:
  preprocess  sqlite3.c: 274642 us
  parse       sqlite3.c: 168252 us
  typecheck   sqlite3.c:  15160 us
  opt         sqlite3.c:  44683 us
  codegen     sqlite3.c: 141618 us

RCC -O2:
  preprocess  sqlite3.c: 276713 us
  parse       sqlite3.c: 163399 us
  typecheck   sqlite3.c:  16642 us
  opt         sqlite3.c: 213459 us
  codegen     sqlite3.c: 151878 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       981 ms |
| RCC -O1   |      1090 ms |
| RCC -O2   |      1264 ms |
| TCC       |       123 ms |
| SLIMCC    |      1450 ms |
| KEFIR     |     25216 ms |
| KEFIR -O1 |     27188 ms |
| CCC       |     18397 ms |
| GCC -O0   |     10873 ms |
| GCC -O2   |     70205 ms |
| Clang -O0 |      3139 ms |
| Clang -O2 |     39376 ms |
