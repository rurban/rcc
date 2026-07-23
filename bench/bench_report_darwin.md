# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          792 |        889 |
| RCC -O1   |           71 |          737 |        808 |
| RCC -O2   |           86 |          725 |        811 |
| TCC       |           84 |          714 |        798 |
| GCC -O0   |          156 |          565 |        721 |
| GCC -O2   |          196 |          336 |        532 |
| Clang -O0 |           99 |          501 |        600 |
| Clang -O2 |          110 |          322 |        432 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1795 us
  parse       bench.c:    216 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    165 us
  link        bench_rcc:  60124 us

RCC -O1:
  preprocess  bench.c:    666 us
  parse       bench.c:    128 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    154 us
  link        bench_rcc_o1:  64687 us

RCC -O2:
  preprocess  bench.c:    648 us
  parse       bench.c:    129 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    115 us
  link        bench_rcc_o2:  68211 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 435155 us
  parse       sqlite3.c: 174108 us
  typecheck   sqlite3.c:  29851 us
  codegen     sqlite3.c:  96646 us

RCC -O1:
  preprocess  sqlite3.c: 384878 us
  parse       sqlite3.c:  62442 us
  typecheck   sqlite3.c:  26320 us
  opt         sqlite3.c:  24154 us
  codegen     sqlite3.c:  82076 us

RCC -O2:
  preprocess  sqlite3.c: 326706 us
  parse       sqlite3.c:  54665 us
  typecheck   sqlite3.c:  23602 us
  opt         sqlite3.c: 188936 us
  codegen     sqlite3.c:  83542 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       659 ms |
| RCC -O1   |       476 ms |
| RCC -O2   |       670 ms |
| TCC       |       100 ms |
| GCC -O0   |      1323 ms |
| GCC -O2   |     13883 ms |
| Clang -O0 |      2833 ms |
| Clang -O2 |     17955 ms |
