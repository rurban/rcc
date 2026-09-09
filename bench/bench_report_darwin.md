# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           92 |          666 |        758 |    48% |
| RCC -O1   |           59 |          641 |        700 |    10% |
| RCC -O2   |           60 |          641 |        701 |     1% |
| TCC       |           25 |          554 |        579 |   128% |
| GCC -O0   |           71 |          468 |        539 |    14% |
| GCC -O2   |          100 |          282 |        382 |    11% |
| Clang -O0 |           60 |          467 |        527 |     7% |
| Clang -O2 |          113 |          295 |        408 |    29% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          165 |         4923 |       5088 |    16% |
| RCC -O1   |          130 |         4812 |       4942 |    23% |
| RCC -O2   |          131 |         4306 |       4437 |     6% |
| TCC       |          110 |         4120 |       4230 |     7% |
| GCC -O0   |          506 |         3385 |       3891 |     8% |
| GCC -O2   |          863 |         1930 |       2793 |    13% |
| Clang -O0 |          496 |         3413 |       3909 |     7% |
| Clang -O2 |          894 |         1928 |       2822 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    862 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    174 us
  link        bench_rcc     :    166 us
  link        bench_rcc     :  85492 us

RCC -O1:
  preprocess  bench.c       :    825 us
  parse       bench.c       :    250 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    171 us
  link        bench_o1      :    187 us
  link        bench_o1      :  77749 us

RCC -O2:
  preprocess  bench.c       :    744 us
  parse       bench.c       :    185 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    173 us
  link        bench_o2      :    201 us
  link        bench_o2      :  78984 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 370705 us
  parse       sqlite3.c     :  89004 us
  typecheck   sqlite3.c     :  19247 us
  codegen     sqlite3.c     : 196668 us
  link        sqlite3.so    :  22081 us

RCC -O1:
  preprocess  sqlite3.c     : 320082 us
  parse       sqlite3.c     :  71688 us
  typecheck   sqlite3.c     :  16990 us
  opt         sqlite3.c     :  48646 us
  codegen     sqlite3.c     : 201148 us
  link        sqlite3.so    :  30684 us

RCC -O2:
  preprocess  sqlite3.c     : 347355 us
  parse       sqlite3.c     :  64027 us
  typecheck   sqlite3.c     :  12930 us
  opt         sqlite3.c     :  31310 us
  codegen     sqlite3.c     : 125618 us
  link        sqlite3.so    :  16301 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       424 ms |     1% |
| RCC -O1   |       429 ms |     0% |
| RCC -O2   |       436 ms |     6% |
| TCC       |       108 ms |     4% |
| GCC -O0   |      1036 ms |     2% |
| GCC -O2   |      9530 ms |     4% |
| Clang -O0 |       993 ms |     4% |
| Clang -O2 |      9585 ms |     1% |
