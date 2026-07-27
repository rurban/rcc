# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          576 |        642 |
| RCC -O1   |           52 |          630 |        682 |
| RCC -O2   |           51 |          633 |        684 |
| TCC       |           39 |          516 |        555 |
| GCC -O0   |           59 |          437 |        496 |
| GCC -O2   |           90 |          265 |        355 |
| Clang -O0 |           55 |          439 |        494 |
| Clang -O2 |           86 |          264 |        350 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    560 us
  parse       bench.c:    115 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    103 us
  link        bench_rcc:  47220 us

RCC -O1:
  preprocess  bench.c:    493 us
  parse       bench.c:    110 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    104 us
  link        bench_rcc_o1:  46599 us

RCC -O2:
  preprocess  bench.c:    506 us
  parse       bench.c:    102 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    103 us
  link        bench_rcc_o2:  50856 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 226642 us
  parse       sqlite3.c:  40860 us
  typecheck   sqlite3.c:  16558 us
  codegen     sqlite3.c:  41655 us

RCC -O1:
  preprocess  sqlite3.c: 185323 us
  parse       sqlite3.c:  46581 us
  typecheck   sqlite3.c:  14335 us
  opt         sqlite3.c:  22548 us
  codegen     sqlite3.c:  47896 us

RCC -O2:
  preprocess  sqlite3.c: 225010 us
  parse       sqlite3.c:  44728 us
  typecheck   sqlite3.c:  16993 us
  opt         sqlite3.c: 124495 us
  codegen     sqlite3.c:  43176 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       486 ms |
| RCC -O1   |       486 ms |
| RCC -O2   |       590 ms |
| TCC       |        67 ms |
| GCC -O0   |       902 ms |
| GCC -O2   |      9914 ms |
| Clang -O0 |       961 ms |
| Clang -O2 |      9032 ms |
