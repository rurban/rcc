# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           81 |          752 |        833 |    48% |
| RCC -O1   |           72 |          710 |        782 |     7% |
| RCC -O2   |           65 |          746 |        811 |    12% |
| TCC       |           35 |          546 |        581 |   157% |
| GCC -O0   |           68 |          441 |        509 |    17% |
| GCC -O2   |           92 |          271 |        363 |     5% |
| Clang -O0 |           60 |          439 |        499 |    20% |
| Clang -O2 |          109 |          283 |        392 |    19% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          137 |         5392 |       5529 |    14% |
| RCC -O1   |          128 |         5318 |       5446 |    89% |
| RCC -O2   |          129 |         4613 |       4742 |   139% |
| TCC       |          122 |         3862 |       3984 |    42% |
| GCC -O0   |          534 |         3290 |       3824 |    28% |
| GCC -O2   |          802 |         1799 |       2601 |     7% |
| Clang -O0 |          461 |         3184 |       3645 |    11% |
| Clang -O2 |          793 |         1843 |       2636 |    28% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    978 us
  parse       bench.c       :    126 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    118 us
  link        bench_rcc     :     70 us
  link        bench_rcc     :  77040 us

RCC -O1:
  preprocess  bench.c       :    670 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    138 us
  link        bench_o1      :    152 us
  link        bench_o1      :  63985 us

RCC -O2:
  preprocess  bench.c       :   1081 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :     40 us
  opt         bench.c       :     76 us
  codegen     bench.c       :    155 us
  link        bench_o2      :    677 us
  link        bench_o2      :  73718 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 338466 us
  parse       sqlite3.c     : 314598 us
  typecheck   sqlite3.c     :  24317 us
  codegen     sqlite3.c     : 221594 us
  link        sqlite3.so    :  20473 us

RCC -O1:
  preprocess  sqlite3.c     : 323252 us
  parse       sqlite3.c     :  91575 us
  typecheck   sqlite3.c     :  14883 us
  opt         sqlite3.c     : 105402 us
  codegen     sqlite3.c     : 322435 us
  link        sqlite3.so    :  28246 us

RCC -O2:
  preprocess  sqlite3.c     : 428261 us
  parse       sqlite3.c     :  91940 us
  typecheck   sqlite3.c     :  22982 us
  opt         sqlite3.c     :  72483 us
  codegen     sqlite3.c     : 261003 us
  link        sqlite3.so    :  23143 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       457 ms |    59% |
| RCC -O1   |       427 ms |    14% |
| RCC -O2   |       432 ms |     2% |
| TCC       |        84 ms |    29% |
| GCC -O0   |       988 ms |     7% |
| GCC -O2   |      9419 ms |     0% |
| Clang -O0 |       929 ms |    15% |
| Clang -O2 |      9939 ms |     6% |
