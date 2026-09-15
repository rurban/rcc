# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           61 |          742 |        803 |    31% |
| RCC -O1   |          108 |          797 |        905 |    63% |
| RCC -O2   |           66 |          935 |       1001 |    28% |
| TCC       |           60 |          752 |        812 |    96% |
| GCC -O0   |           94 |          588 |        682 |    71% |
| GCC -O2   |          194 |          408 |        602 |    15% |
| Clang -O0 |          145 |          628 |        773 |    40% |
| Clang -O2 |          157 |          371 |        528 |   161% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          176 |         7131 |       7307 |   156% |
| RCC -O1   |          268 |         6298 |       6566 |    56% |
| RCC -O2   |          171 |         4835 |       5006 |   156% |
| TCC       |          123 |         5120 |       5243 |   169% |
| GCC -O0   |          575 |         5087 |       5662 |     9% |
| GCC -O2   |         1284 |         2202 |       3486 |    48% |
| Clang -O0 |          639 |         3907 |       4546 |    41% |
| Clang -O2 |          973 |         2393 |       3366 |    15% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   3091 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :    101 us
  link        bench_rcc     :  65349 us

RCC -O1:
  preprocess  bench.c       :    583 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    125 us
  link        bench_o1      :    106 us
  link        bench_o1      :  52156 us

RCC -O2:
  preprocess  bench.c       :    680 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    123 us
  link        bench_o2      :    123 us
  link        bench_o2      :  51560 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 385101 us
  parse       sqlite3.c     : 307250 us
  typecheck   sqlite3.c     :  35972 us
  codegen     sqlite3.c     : 271869 us
  link        sqlite3.so    :  16069 us

RCC -O1:
  preprocess  sqlite3.c     : 345320 us
  parse       sqlite3.c     :  63195 us
  typecheck   sqlite3.c     :  22940 us
  opt         sqlite3.c     :  53610 us
  codegen     sqlite3.c     : 190397 us
  link        sqlite3.so    :  21316 us

RCC -O2:
  preprocess  sqlite3.c     : 249508 us
  parse       sqlite3.c     :  55895 us
  typecheck   sqlite3.c     :  11818 us
  opt         sqlite3.c     :  34466 us
  codegen     sqlite3.c     : 133324 us
  link        sqlite3.so    :  18031 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       680 ms |    39% |
| RCC -O1   |       616 ms |     0% |
| RCC -O2   |       586 ms |     4% |
| TCC       |       109 ms |    48% |
| GCC -O0   |      1293 ms |     5% |
| GCC -O2   |     11802 ms |    11% |
| Clang -O0 |      1353 ms |    22% |
| Clang -O2 |     15174 ms |     4% |
