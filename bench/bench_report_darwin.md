# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          144 |          799 |        943 |
| RCC -O1   |           87 |          798 |        885 |
| RCC -O2   |          120 |          820 |        940 |
| TCC       |          136 |          786 |        922 |
| GCC -O0   |          204 |          733 |        937 |
| GCC -O2   |          266 |          340 |        606 |
| Clang -O0 |           95 |          606 |        701 |
| Clang -O2 |          165 |          314 |        479 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          254 |         7574 |       7828 |
| RCC -O1   |          336 |         5377 |       5713 |
| RCC -O2   |          274 |         6356 |       6630 |
| TCC       |          223 |         5299 |       5522 |
| GCC -O0   |          851 |         3976 |       4827 |
| GCC -O2   |         1167 |         2267 |       3434 |
| Clang -O0 |          678 |         3846 |       4524 |
| Clang -O2 |         1039 |         2282 |       3321 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2282 us
  parse       bench.c       :    469 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    354 us
  link        bench_rcc     :    283 us
  link        bench_rcc     : 105379 us

RCC -O1:
  preprocess  bench.c       :   1634 us
  parse       bench.c       :    476 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     68 us
  codegen     bench.c       :    384 us
  link        bench_o1      :    284 us
  link        bench_o1      :  92118 us

RCC -O2:
  preprocess  bench.c       :   1593 us
  parse       bench.c       :    985 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     56 us
  codegen     bench.c       :    345 us
  link        bench_o2      :    502 us
  link        bench_o2      : 101431 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 657450 us
  parse       sqlite3.c     : 139293 us
  typecheck   sqlite3.c     :  29552 us
  codegen     sqlite3.c     : 186492 us
  link        sqlite3.so    :  20796 us

RCC -O1:
  preprocess  sqlite3.c     : 329664 us
  parse       sqlite3.c     :  77468 us
  typecheck   sqlite3.c     :  22992 us
  opt         sqlite3.c     : 273632 us
  codegen     sqlite3.c     : 190522 us
  link        sqlite3.so    :  21708 us

RCC -O2:
  preprocess  sqlite3.c     : 340715 us
  parse       sqlite3.c     :  77697 us
  typecheck   sqlite3.c     :  13887 us
  opt         sqlite3.c     : 279079 us
  codegen     sqlite3.c     : 150295 us
  link        sqlite3.so    :  24774 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1200 ms |
| RCC -O1   |       855 ms |
| RCC -O2   |       830 ms |
| TCC       |       140 ms |
| GCC -O0   |      1238 ms |
| GCC -O2   |     12980 ms |
| Clang -O0 |      1494 ms |
| Clang -O2 |     11976 ms |
