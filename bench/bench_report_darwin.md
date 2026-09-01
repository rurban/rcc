# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          159 |          824 |        983 |
| RCC -O1   |           89 |          829 |        918 |
| RCC -O2   |          133 |          893 |       1026 |
| TCC       |           91 |          770 |        861 |
| GCC -O0   |          110 |          561 |        671 |
| GCC -O2   |          143 |          332 |        475 |
| Clang -O0 |          127 |          599 |        726 |
| Clang -O2 |          245 |          375 |        620 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          344 |         5752 |       6096 |
| RCC -O1   |          256 |         5299 |       5555 |
| RCC -O2   |          199 |         4650 |       4849 |
| TCC       |          168 |         5687 |       5855 |
| GCC -O0   |          942 |         3535 |       4477 |
| GCC -O2   |         1212 |         2159 |       3371 |
| Clang -O0 |          721 |         3916 |       4637 |
| Clang -O2 |         1110 |         2210 |       3320 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2358 us
  parse       bench.c       :    751 us
  typecheck   bench.c       :     12 us
  codegen     bench.c       :    329 us
  link        bench_rcc     :    394 us
  link        bench_rcc     :  94345 us

RCC -O1:
  preprocess  bench.c       :    697 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    128 us
  link        bench_o1      :    168 us
  link        bench_o1      :  74926 us

RCC -O2:
  preprocess  bench.c       :    660 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    159 us
  link        bench_o2      :    214 us
  link        bench_o2      :  71732 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 380632 us
  parse       sqlite3.c     : 108180 us
  typecheck   sqlite3.c     :  24191 us
  codegen     sqlite3.c     : 161600 us
  link        sqlite3.so    :  22358 us

RCC -O1:
  preprocess  sqlite3.c     : 444682 us
  parse       sqlite3.c     :  75838 us
  typecheck   sqlite3.c     :  20305 us
  opt         sqlite3.c     : 284046 us
  codegen     sqlite3.c     : 171356 us
  link        sqlite3.so    :  17989 us

RCC -O2:
  preprocess  sqlite3.c     : 1040659 us
  parse       sqlite3.c     : 519369 us
  typecheck   sqlite3.c     :  40807 us
  opt         sqlite3.c     : 532791 us
  codegen     sqlite3.c     : 304417 us
  link        sqlite3.so    :  52682 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1413 ms |
| RCC -O1   |       949 ms |
| RCC -O2   |       797 ms |
| TCC       |       148 ms |
| GCC -O0   |      1295 ms |
| GCC -O2   |     11959 ms |
| Clang -O0 |      1276 ms |
| Clang -O2 |     10881 ms |
