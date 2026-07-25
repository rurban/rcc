# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          103 |          787 |        890 |
| RCC -O1   |          108 |          832 |        940 |
| RCC -O2   |          143 |          829 |        972 |
| TCC       |          123 |          702 |        825 |
| GCC -O0   |          123 |          612 |        735 |
| GCC -O2   |          150 |          352 |        502 |
| Clang -O0 |           98 |          620 |        718 |
| Clang -O2 |          175 |          341 |        516 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1675 us
  parse       bench.c:    277 us
  typecheck   bench.c:      9 us
  codegen     bench.c:    263 us
  link        bench_rcc:  80559 us

RCC -O1:
  preprocess  bench.c:   1704 us
  parse       bench.c:    148 us
  typecheck   bench.c:      5 us
  opt         bench.c:     28 us
  codegen     bench.c:    162 us
  link        bench_rcc_o1:  71462 us

RCC -O2:
  preprocess  bench.c:    609 us
  parse       bench.c:    138 us
  typecheck   bench.c:      4 us
  opt         bench.c:     18 us
  codegen     bench.c:    111 us
  link        bench_rcc_o2:  79572 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 380262 us
  parse       sqlite3.c: 139652 us
  typecheck   sqlite3.c:  28293 us
  codegen     sqlite3.c:  68895 us

RCC -O1:
  preprocess  sqlite3.c: 340549 us
  parse       sqlite3.c:  62200 us
  typecheck   sqlite3.c:  19547 us
  opt         sqlite3.c:  21753 us
  codegen     sqlite3.c:  75208 us

RCC -O2:
  preprocess  sqlite3.c: 341528 us
  parse       sqlite3.c:  76909 us
  typecheck   sqlite3.c:  19283 us
  opt         sqlite3.c: 251816 us
  codegen     sqlite3.c:  66492 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1034 ms |
| RCC -O1   |       860 ms |
| RCC -O2   |      1012 ms |
| TCC       |       122 ms |
| GCC -O0   |      1570 ms |
| GCC -O2   |     18013 ms |
| Clang -O0 |      1903 ms |
| Clang -O2 |     16646 ms |
