# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           63 |          688 |        751 |    28% |
| RCC -O1   |           72 |          672 |        744 |    11% |
| RCC -O2   |           71 |          702 |        773 |    35% |
| TCC       |           27 |          635 |        662 |    66% |
| GCC -O0   |           97 |          544 |        641 |     9% |
| GCC -O2   |          137 |          335 |        472 |    28% |
| Clang -O0 |           73 |          602 |        675 |    53% |
| Clang -O2 |          139 |          317 |        456 |    25% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          201 |         6418 |       6619 |    73% |
| RCC -O1   |          241 |         7101 |       7342 |   104% |
| RCC -O2   |          179 |         6391 |       6570 |    48% |
| TCC       |          133 |         6057 |       6190 |    55% |
| GCC -O0   |          627 |         5297 |       5924 |    41% |
| GCC -O2   |         1071 |         2977 |       4048 |    39% |
| Clang -O0 |          582 |         4739 |       5321 |    13% |
| Clang -O2 |          990 |         2417 |       3407 |    33% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    808 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    171 us
  link        bench_rcc     :    111 us
  link        bench_rcc     :  55790 us

RCC -O1:
  preprocess  bench.c       :    621 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    133 us
  link        bench_o1      :     87 us
  link        bench_o1      :  53326 us

RCC -O2:
  preprocess  bench.c       :    636 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    141 us
  link        bench_o2      :    133 us
  link        bench_o2      :  55047 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 278942 us
  parse       sqlite3.c     : 103412 us
  typecheck   sqlite3.c     :  24284 us
  codegen     sqlite3.c     : 158680 us
  link        sqlite3.so    :  19204 us

RCC -O1:
  preprocess  sqlite3.c     : 265172 us
  parse       sqlite3.c     :  75277 us
  typecheck   sqlite3.c     :  20781 us
  opt         sqlite3.c     :  24758 us
  codegen     sqlite3.c     : 109664 us
  link        sqlite3.so    :  14840 us

RCC -O2:
  preprocess  sqlite3.c     : 240978 us
  parse       sqlite3.c     :  62828 us
  typecheck   sqlite3.c     :  15176 us
  opt         sqlite3.c     :  39765 us
  codegen     sqlite3.c     : 161428 us
  link        sqlite3.so    :  18599 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       692 ms |    75% |
| RCC -O1   |       756 ms |     1% |
| RCC -O2   |       792 ms |     2% |
| TCC       |       158 ms |     1% |
| GCC -O0   |      1848 ms |     6% |
| GCC -O2   |     14188 ms |    29% |
| Clang -O0 |      1287 ms |    22% |
| Clang -O2 |     10972 ms |    19% |
