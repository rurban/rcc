# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          105 |          768 |        873 |
| RCC -O1   |           97 |          833 |        930 |
| RCC -O2   |          119 |          822 |        941 |
| TCC       |          160 |          693 |        853 |
| GCC -O0   |          147 |          563 |        710 |
| GCC -O2   |          188 |          357 |        545 |
| Clang -O0 |          104 |          611 |        715 |
| Clang -O2 |          138 |          347 |        485 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1677 us
  parse       bench.c:    294 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    271 us
  link        bench_rcc:  76022 us

RCC -O1:
  preprocess  bench.c:   1476 us
  parse       bench.c:    129 us
  typecheck   bench.c:      6 us
  opt         bench.c:     22 us
  codegen     bench.c:    141 us
  link        bench_rcc_o1:  76283 us

RCC -O2:
  preprocess  bench.c:   1298 us
  parse       bench.c:    144 us
  typecheck   bench.c:      6 us
  opt         bench.c:     24 us
  codegen     bench.c:    129 us
  link        bench_rcc_o2:  89413 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 507833 us
  parse       sqlite3.c: 141977 us
  typecheck   sqlite3.c:  23005 us
  codegen     sqlite3.c:  73553 us

RCC -O1:
  preprocess  sqlite3.c: 266882 us
  parse       sqlite3.c:  61700 us
  typecheck   sqlite3.c:  15426 us
  opt         sqlite3.c:  24597 us
  codegen     sqlite3.c:  79729 us

RCC -O2:
  preprocess  sqlite3.c: 291138 us
  parse       sqlite3.c:  51527 us
  typecheck   sqlite3.c:  18678 us
  opt         sqlite3.c: 223852 us
  codegen     sqlite3.c:  87165 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1196 ms |
| RCC -O1   |       879 ms |
| RCC -O2   |      1055 ms |
| TCC       |       124 ms |
| GCC -O0   |      1568 ms |
| GCC -O2   |     15816 ms |
| Clang -O0 |      1673 ms |
| Clang -O2 |     15185 ms |
