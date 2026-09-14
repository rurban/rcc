# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           72 |          727 |        799 |    43% |
| RCC -O1   |           66 |          770 |        836 |    36% |
| RCC -O2   |           72 |          745 |        817 |    25% |
| TCC       |           47 |          701 |        748 |    72% |
| GCC -O0   |          100 |          588 |        688 |    67% |
| GCC -O2   |          187 |          372 |        559 |    26% |
| Clang -O0 |           98 |          675 |        773 |    13% |
| Clang -O2 |          259 |          371 |        630 |   105% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          286 |         6436 |       6722 |    47% |
| RCC -O1   |          129 |         6858 |       6987 |    97% |
| RCC -O2   |          251 |         4852 |       5103 |   217% |
| TCC       |          124 |         4287 |       4411 |    85% |
| GCC -O0   |          580 |         3916 |       4496 |    25% |
| GCC -O2   |          955 |         2198 |       3153 |    22% |
| Clang -O0 |          486 |         3437 |       3923 |    26% |
| Clang -O2 |          815 |         1845 |       2660 |    16% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    896 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    186 us
  link        bench_rcc     :    349 us
  link        bench_rcc     :  68918 us

RCC -O1:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    138 us
  link        bench_o1      :    496 us
  link        bench_o1      :  65508 us

RCC -O2:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    150 us
  link        bench_o2      :    551 us
  link        bench_o2      :  72104 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 307750 us
  parse       sqlite3.c     :  66812 us
  typecheck   sqlite3.c     :  19254 us
  codegen     sqlite3.c     : 167175 us
  link        sqlite3.so    :  20312 us

RCC -O1:
  preprocess  sqlite3.c     : 292499 us
  parse       sqlite3.c     :  72063 us
  typecheck   sqlite3.c     :  14589 us
  opt         sqlite3.c     :  34264 us
  codegen     sqlite3.c     : 218268 us
  link        sqlite3.so    :  30125 us

RCC -O2:
  preprocess  sqlite3.c     : 405200 us
  parse       sqlite3.c     :  66419 us
  typecheck   sqlite3.c     :  11514 us
  opt         sqlite3.c     :  36201 us
  codegen     sqlite3.c     : 140552 us
  link        sqlite3.so    :  18633 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       439 ms |    60% |
| RCC -O1   |       425 ms |    17% |
| RCC -O2   |       414 ms |     4% |
| TCC       |       103 ms |     1% |
| GCC -O0   |      1064 ms |     1% |
| GCC -O2   |      9349 ms |    17% |
| Clang -O0 |      1153 ms |     4% |
| Clang -O2 |     11208 ms |     0% |
