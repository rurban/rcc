# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          123 |          731 |        854 |
| RCC -O1   |           86 |          689 |        775 |
| RCC -O2   |           77 |          682 |        759 |
| TCC       |           67 |          686 |        753 |
| GCC -O0   |          156 |          533 |        689 |
| GCC -O2   |          159 |          308 |        467 |
| Clang -O0 |          133 |          501 |        634 |
| Clang -O2 |          155 |          320 |        475 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    886 us
  parse       bench.c:    205 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    175 us
  link        bench_rcc:  67517 us

RCC -O1:
  preprocess  bench.c:    691 us
  parse       bench.c:    317 us
  typecheck   bench.c:      9 us
  opt         bench.c:     47 us
  codegen     bench.c:    307 us
  link        bench_rcc_o1:  68453 us

RCC -O2:
  preprocess  bench.c:    666 us
  parse       bench.c:    150 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    123 us
  link        bench_rcc_o2:  67181 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 314877 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c:  66238 us

RCC -O1:
  preprocess  sqlite3.c: 292782 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c:  59703 us

RCC -O2:
  preprocess  sqlite3.c: 333123 us
sqlite3.c:143626: [0m[1;31merror:[0m initializing an array of incompatible element type with a string literal
      "dylib"
      [1;31m^~~~~[0m
  parse       sqlite3.c:  80948 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        89 ms |
| GCC -O0   |      1115 ms |
| GCC -O2   |     12166 ms |
| Clang -O0 |      1346 ms |
| Clang -O2 |     11263 ms |
