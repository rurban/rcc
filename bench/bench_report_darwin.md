# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           91 |          667 |        758 |
| RCC -O1   |           63 |          672 |        735 |
| RCC -O2   |           73 |          768 |        841 |
| TCC       |           71 |          802 |        873 |
| GCC -O0   |          204 |          691 |        895 |
| GCC -O2   |          240 |          416 |        656 |
| Clang -O0 |          160 |          729 |        889 |
| Clang -O2 |          245 |          391 |        636 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          499 |         6540 |       7039 |
| RCC -O1   |          183 |         6137 |       6320 |
| RCC -O2   |          322 |         7248 |       7570 |
| TCC       |          210 |         5664 |       5874 |
| GCC -O0   |          718 |         5126 |       5844 |
| GCC -O2   |         1873 |         2652 |       4525 |
| Clang -O0 |          804 |         3854 |       4658 |
| Clang -O2 |         1011 |         2005 |       3016 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1081 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    161 us
  link        bench_rcc     :    224 us
  link        bench_rcc     :  64014 us

RCC -O1:
  preprocess  bench.c       :    703 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    129 us
  link        bench_o1      :    362 us
  link        bench_o1      :  65648 us

RCC -O2:
  preprocess  bench.c       :    901 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    196 us
  link        bench_o2      :    236 us
  link        bench_o2      :  79162 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 364317 us
  parse       sqlite3.c     : 122120 us
  typecheck   sqlite3.c     :  22725 us
  codegen     sqlite3.c     : 149949 us
  link        sqlite3.so    :  30614 us

RCC -O1:
  preprocess  sqlite3.c     : 438911 us
  parse       sqlite3.c     :  61393 us
  typecheck   sqlite3.c     :  13457 us
  opt         sqlite3.c     : 181455 us
  codegen     sqlite3.c     : 225190 us
  link        sqlite3.so    :  21349 us

RCC -O2:
  preprocess  sqlite3.c     : 281180 us
  parse       sqlite3.c     :  62607 us
  typecheck   sqlite3.c     :  14773 us
  opt         sqlite3.c     : 186020 us
  codegen     sqlite3.c     : 153351 us
  link        sqlite3.so    :  26431 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1862 ms |
| RCC -O1   |       887 ms |
| RCC -O2   |      1018 ms |
| TCC       |       133 ms |
| GCC -O0   |      1195 ms |
| GCC -O2   |     12154 ms |
| Clang -O0 |      1120 ms |
| Clang -O2 |     11410 ms |
