# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           70 |          665 |        735 |    20% |
| RCC -O1   |           73 |          653 |        726 |     6% |
| RCC -O2   |           69 |          668 |        737 |    10% |
| TCC       |           33 |          564 |        597 |    84% |
| GCC -O0   |           81 |          476 |        557 |    14% |
| GCC -O2   |          110 |          285 |        395 |    13% |
| Clang -O0 |           72 |          479 |        551 |     1% |
| Clang -O2 |          106 |          287 |        393 |    14% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          189 |         5174 |       5363 |    16% |
| RCC -O1   |          202 |         5047 |       5249 |    34% |
| RCC -O2   |          175 |         4848 |       5023 |     3% |
| TCC       |          123 |         4835 |       4958 |    16% |
| GCC -O0   |          606 |         3778 |       4384 |    12% |
| GCC -O2   |          892 |         2035 |       2927 |    30% |
| Clang -O0 |          608 |         3586 |       4194 |    10% |
| Clang -O2 |         1018 |         2320 |       3338 |    13% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    925 us
  parse       bench.c       :    218 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    279 us
  link        bench_rcc     :    199 us
  link        bench_rcc     :  73689 us

RCC -O1:
  preprocess  bench.c       :    664 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     54 us
  codegen     bench.c       :    166 us
  link        bench_o1      :    198 us
  link        bench_o1      :  83901 us

RCC -O2:
  preprocess  bench.c       :   1408 us
  parse       bench.c       :    313 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     98 us
  codegen     bench.c       :    292 us
  link        bench_o2      :    260 us
  link        bench_o2      :  78318 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 274588 us
  parse       sqlite3.c     :  69543 us
  typecheck   sqlite3.c     :  13544 us
  codegen     sqlite3.c     : 136268 us
  link        sqlite3.so    :  18873 us

RCC -O1:
  preprocess  sqlite3.c     : 228810 us
  parse       sqlite3.c     :  59347 us
  typecheck   sqlite3.c     :  12162 us
  opt         sqlite3.c     :  49703 us
  codegen     sqlite3.c     : 118305 us
  link        sqlite3.so    :  20597 us

RCC -O2:
  preprocess  sqlite3.c     : 263150 us
  parse       sqlite3.c     :  65100 us
  typecheck   sqlite3.c     :  18519 us
  opt         sqlite3.c     :  63010 us
  codegen     sqlite3.c     : 127108 us
  link        sqlite3.so    :  17990 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       570 ms |    23% |
| RCC -O1   |       494 ms |     2% |
| RCC -O2   |       543 ms |    15% |
| TCC       |       112 ms |    21% |
| GCC -O0   |      1575 ms |     5% |
| GCC -O2   |     13932 ms |     3% |
| Clang -O0 |      1386 ms |     9% |
| Clang -O2 |     13552 ms |     4% |
