# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           33 |          721 |        754 |    12% |
| RCC -O1   |           32 |          679 |        711 |    37% |
| RCC -O2   |           30 |          336 |        366 |    30% |
| RCC -O3   |           33 |          340 |        373 |    33% |
| TCC       |           25 |          535 |        560 |    40% |
| GCC -O0   |           63 |          473 |        536 |    20% |
| GCC -O2   |           96 |          272 |        368 |     9% |
| Clang -O0 |           59 |          480 |        539 |     3% |
| Clang -O2 |           90 |          266 |        356 |     3% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          135 |         4377 |       4512 |     7% |
| RCC -O1   |          124 |         4559 |       4683 |     8% |
| RCC -O2   |          167 |         2116 |       2283 |    11% |
| RCC -O3   |          221 |         2044 |       2265 |     4% |
| TCC       |           99 |         3905 |       4004 |    34% |
| GCC -O0   |          499 |         3093 |       3592 |    13% |
| GCC -O2   |          817 |         1782 |       2599 |    12% |
| Clang -O0 |          492 |         3138 |       3630 |     3% |
| Clang -O2 |          826 |         1795 |       2621 |     3% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    974 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    141 us
  link        bench_rcc     :  27886 us

RCC -O1:
  preprocess  bench.c       :    612 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    302 us
  link        bench_o1      :  28021 us

RCC -O2:
  preprocess  bench.c       :    582 us
  parse       bench.c       :    177 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     54 us
  codegen     bench.c       :   1584 us
  link        bench_o2      :  27356 us

RCC -O3:
  preprocess  bench.c       :    612 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     56 us
  codegen     bench.c       :   1596 us
  link        bench_o3      :  27897 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 181721 us
  parse       sqlite3.c     :  45900 us
  typecheck   sqlite3.c     :  13669 us
  codegen     sqlite3.c     : 137226 us
  link        sqlite3.so    :  44106 us

RCC -O1:
  preprocess  sqlite3.c     : 214445 us
  parse       sqlite3.c     :  51261 us
  typecheck   sqlite3.c     :  13430 us
  opt         sqlite3.c     :  46818 us
  codegen     sqlite3.c     : 114577 us
  link        sqlite3.so    :  30754 us

RCC -O2:
  preprocess  sqlite3.c     : 204557 us
  parse       sqlite3.c     :  55825 us
  typecheck   sqlite3.c     :  16024 us
  opt         sqlite3.c     :  62710 us
  codegen     sqlite3.c     : 1002298 us
  link        sqlite3.so    :  28831 us

RCC -O3:
  preprocess  sqlite3.c     : 183109 us
  parse       sqlite3.c     :  53139 us
  typecheck   sqlite3.c     :  17974 us
  opt         sqlite3.c     :  54299 us
  codegen     sqlite3.c     : 969701 us
  link        sqlite3.so    :  28369 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       471 ms |    28% |
| RCC -O1   |       473 ms |     1% |
| RCC -O2   |      1144 ms |     3% |
| RCC -O3   |      1150 ms |     0% |
| TCC       |        84 ms |     1% |
| GCC -O0   |      1031 ms |    15% |
| GCC -O2   |      8877 ms |     0% |
| Clang -O0 |       898 ms |     0% |
| Clang -O2 |      8778 ms |     1% |
