# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           50 |          917 |        967 |    42% |
| RCC -O1   |           59 |          839 |        898 |    28% |
| RCC -O2   |           43 |          390 |        433 |    58% |
| RCC -O3   |           39 |          450 |        489 |    71% |
| TCC       |           42 |          719 |        761 |    95% |
| GCC -O0   |          112 |          642 |        754 |    46% |
| GCC -O2   |          159 |          360 |        519 |    17% |
| Clang -O0 |           93 |          626 |        719 |    25% |
| Clang -O2 |          137 |          367 |        504 |    21% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          213 |         5992 |       6205 |    69% |
| RCC -O1   |          223 |         5808 |       6031 |    81% |
| RCC -O2   |          279 |         3067 |       3346 |    17% |
| RCC -O3   |          227 |         2839 |       3066 |    31% |
| TCC       |          142 |         5574 |       5716 |    59% |
| GCC -O0   |          702 |         4170 |       4872 |    13% |
| GCC -O2   |         1075 |         2548 |       3623 |    29% |
| Clang -O0 |          592 |         4119 |       4711 |    27% |
| Clang -O2 |         1021 |         2657 |       3678 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1248 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    184 us
  link        bench_rcc     :  31751 us

RCC -O1:
  preprocess  bench.c       :    680 us
  parse       bench.c       :    208 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     49 us
  codegen     bench.c       :    171 us
  link        bench_o1      :  29635 us

RCC -O2:
  preprocess  bench.c       :    634 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     54 us
  codegen     bench.c       :   1746 us
  link        bench_o2      :  34604 us

RCC -O3:
  preprocess  bench.c       :    573 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     55 us
  codegen     bench.c       :   1587 us
  link        bench_o3      :  29536 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 384865 us
  parse       sqlite3.c     : 297173 us
  typecheck   sqlite3.c     :  35557 us
  codegen     sqlite3.c     : 310065 us
  link        sqlite3.so    :  55362 us

RCC -O1:
  preprocess  sqlite3.c     : 399151 us
  parse       sqlite3.c     : 118495 us
  typecheck   sqlite3.c     :  28201 us
  opt         sqlite3.c     :  86242 us
  codegen     sqlite3.c     : 261114 us
  link        sqlite3.so    :  62214 us

RCC -O2:
  preprocess  sqlite3.c     : 435400 us
  parse       sqlite3.c     :  95439 us
  typecheck   sqlite3.c     :  16618 us
  opt         sqlite3.c     :  70351 us
  codegen     sqlite3.c     : 1585388 us
  link        sqlite3.so    : 136127 us

RCC -O3:
  preprocess  sqlite3.c     : 553409 us
  parse       sqlite3.c     : 260838 us
  typecheck   sqlite3.c     :  33705 us
  opt         sqlite3.c     :  81921 us
  codegen     sqlite3.c     : 1708399 us
  link        sqlite3.so    :  56434 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       696 ms |    65% |
| RCC -O1   |       583 ms |    17% |
| RCC -O2   |      1713 ms |     4% |
| RCC -O3   |      1767 ms |     2% |
| TCC       |       147 ms |     6% |
| GCC -O0   |      1564 ms |     2% |
| GCC -O2   |     11496 ms |    28% |
| Clang -O0 |      1473 ms |     5% |
| Clang -O2 |     16161 ms |     8% |
