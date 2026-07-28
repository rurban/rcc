# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          115 |          909 |       1024 |
| RCC -O1   |          103 |          869 |        972 |
| RCC -O2   |          100 |          820 |        920 |
| TCC       |           75 |          795 |        870 |
| GCC -O0   |          139 |          642 |        781 |
| GCC -O2   |          204 |          379 |        583 |
| Clang -O0 |          104 |          639 |        743 |
| Clang -O2 |          169 |          374 |        543 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1713 us
  parse       bench.c:    271 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    273 us
  link        bench_rcc:  96034 us

RCC -O1:
  preprocess  bench.c:    891 us
  parse       bench.c:    130 us
  typecheck   bench.c:      5 us
  opt         bench.c:     22 us
  codegen     bench.c:    187 us
  link        bench_rcc_o1:  79331 us

RCC -O2:
  preprocess  bench.c:   1566 us
  parse       bench.c:    286 us
  typecheck   bench.c:     10 us
  opt         bench.c:     45 us
  codegen     bench.c:    266 us
  link        bench_rcc_o2:  94841 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 593664 us
  parse       sqlite3.c: 200613 us
  typecheck   sqlite3.c:  32442 us
  codegen     sqlite3.c: 116910 us

RCC -O1:
  preprocess  sqlite3.c: 472977 us
  parse       sqlite3.c:  70903 us
  typecheck   sqlite3.c:  32358 us
  opt         sqlite3.c:  37509 us
  codegen     sqlite3.c: 127540 us

RCC -O2:
  preprocess  sqlite3.c: 360706 us
  parse       sqlite3.c:  81934 us
  typecheck   sqlite3.c:  17699 us
  opt         sqlite3.c: 251090 us
  codegen     sqlite3.c:  94297 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1357 ms |
| RCC -O1   |       985 ms |
| RCC -O2   |      1155 ms |
| TCC       |       213 ms |
| GCC -O0   |      1928 ms |
| GCC -O2   |     17767 ms |
| Clang -O0 |      1899 ms |
| Clang -O2 |     18079 ms |
