# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           81 |          738 |        819 |
| RCC -O1   |           70 |          685 |        755 |
| RCC -O2   |           59 |          650 |        709 |
| TCC       |           46 |          569 |        615 |
| GCC -O0   |          106 |          477 |        583 |
| GCC -O2   |          123 |          286 |        409 |
| Clang -O0 |           70 |          472 |        542 |
| Clang -O2 |          111 |          287 |        398 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    669 us
  parse       bench.c:    129 us
  typecheck   bench.c:      3 us
  codegen     bench.c:    137 us
  link        bench_rcc:  71920 us

RCC -O1:
  preprocess  bench.c:    553 us
  parse       bench.c:    111 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    109 us
  link        bench_rcc_o1:  58071 us

RCC -O2:
  preprocess  bench.c:    570 us
  parse       bench.c:    131 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    114 us
  link        bench_rcc_o2:  48286 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 321055 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c: 126021 us

RCC -O1:
  preprocess  sqlite3.c: 273862 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c:  50198 us

RCC -O2:
  preprocess  sqlite3.c: 289408 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c:  57124 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        88 ms |
| GCC -O0   |      1106 ms |
| GCC -O2   |     10634 ms |
| Clang -O0 |      1004 ms |
| Clang -O2 |     11489 ms |
