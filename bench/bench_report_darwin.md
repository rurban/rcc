# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          108 |          859 |        967 |    25% |
| RCC -O1   |          105 |          688 |        793 |    35% |
| RCC -O2   |           72 |          731 |        803 |    19% |
| TCC       |           37 |          644 |        681 |   151% |
| GCC -O0   |           86 |          565 |        651 |    33% |
| GCC -O2   |          126 |          311 |        437 |    23% |
| Clang -O0 |           78 |          519 |        597 |    12% |
| Clang -O2 |           97 |          327 |        424 |    11% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          161 |         6225 |       6386 |    37% |
| RCC -O1   |          182 |         6411 |       6593 |     8% |
| RCC -O2   |          214 |         5741 |       5955 |    48% |
| TCC       |          192 |         5716 |       5908 |    24% |
| GCC -O0   |          629 |         4665 |       5294 |    14% |
| GCC -O2   |         1572 |         2769 |       4341 |    29% |
| Clang -O0 |          711 |         4423 |       5134 |    18% |
| Clang -O2 |         1253 |         2871 |       4124 |    14% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1058 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    133 us
  link        bench_rcc     :    473 us
  link        bench_rcc     :  84183 us

RCC -O1:
  preprocess  bench.c       :    974 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    151 us
  link        bench_o1      :    154 us
  link        bench_o1      :  79789 us

RCC -O2:
  preprocess  bench.c       :    700 us
  parse       bench.c       :    316 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    149 us
  link        bench_o2      :     64 us
  link        bench_o2      : 107957 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 511444 us
  parse       sqlite3.c     :  78338 us
  typecheck   sqlite3.c     :  20067 us
  codegen     sqlite3.c     : 197049 us
  link        sqlite3.so    :  17306 us

RCC -O1:
  preprocess  sqlite3.c     : 398635 us
  parse       sqlite3.c     :  94737 us
  typecheck   sqlite3.c     :  21681 us
  opt         sqlite3.c     :  45351 us
  codegen     sqlite3.c     : 202755 us
  link        sqlite3.so    :  35856 us

RCC -O2:
  preprocess  sqlite3.c     : 383262 us
  parse       sqlite3.c     :  86954 us
  typecheck   sqlite3.c     :  20006 us
  opt         sqlite3.c     :  58283 us
  codegen     sqlite3.c     : 240665 us
  link        sqlite3.so    :  24516 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       737 ms |    22% |
| RCC -O1   |       673 ms |     1% |
| RCC -O2   |       580 ms |     5% |
| TCC       |       148 ms |    12% |
| GCC -O0   |      1502 ms |     3% |
| GCC -O2   |     11292 ms |    23% |
| Clang -O0 |      1142 ms |    20% |
| Clang -O2 |     11047 ms |     4% |
