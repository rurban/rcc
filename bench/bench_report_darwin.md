# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           89 |          660 |        749 |
| RCC -O1   |           54 |          618 |        672 |
| RCC -O2   |           59 |          622 |        681 |
| TCC       |           53 |          563 |        616 |
| GCC -O0   |           90 |          439 |        529 |
| GCC -O2   |          106 |          264 |        370 |
| Clang -O0 |           59 |          434 |        493 |
| Clang -O2 |           95 |          262 |        357 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1109 us
  parse       bench.c:    128 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    114 us
  link        bench_rcc:  49628 us

RCC -O1:
  preprocess  bench.c:    551 us
  parse       bench.c:    112 us
  typecheck   bench.c:      5 us
  opt         bench.c:     17 us
  codegen     bench.c:    132 us
  link        bench_rcc_o1:  55045 us

RCC -O2:
  preprocess  bench.c:    607 us
  parse       bench.c:    112 us
  typecheck   bench.c:      5 us
  opt         bench.c:     23 us
  codegen     bench.c:    111 us
  link        bench_rcc_o2:  46861 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 353622 us
  parse       sqlite3.c: 114310 us
  typecheck   sqlite3.c:  26761 us
  codegen     sqlite3.c:  70907 us

RCC -O1:
  preprocess  sqlite3.c: 269395 us
  parse       sqlite3.c:  54607 us
  typecheck   sqlite3.c:  21748 us
  opt         sqlite3.c:  23009 us
  codegen     sqlite3.c:  52032 us

RCC -O2:
  preprocess  sqlite3.c: 253875 us
  parse       sqlite3.c:  59423 us
  typecheck   sqlite3.c:  30821 us
  opt         sqlite3.c: 204684 us
  codegen     sqlite3.c:  55403 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       358 ms |
| RCC -O1   |       771 ms |
| RCC -O2   |       581 ms |
| TCC       |        85 ms |
| GCC -O0   |       937 ms |
| GCC -O2   |      9156 ms |
| Clang -O0 |       938 ms |
| Clang -O2 |      9071 ms |
