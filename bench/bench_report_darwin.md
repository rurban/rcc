# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          165 |          762 |        927 |
| RCC -O1   |           81 |          681 |        762 |
| RCC -O2   |           82 |          669 |        751 |
| TCC       |           60 |          580 |        640 |
| GCC -O0   |           97 |          493 |        590 |
| GCC -O2   |          129 |          304 |        433 |
| Clang -O0 |           63 |          439 |        502 |
| Clang -O2 |           85 |          263 |        348 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          134 |         4587 |       4721 |
| RCC -O1   |          173 |         4388 |       4561 |
| RCC -O2   |          138 |         3957 |       4095 |
| TCC       |          157 |         3698 |       3855 |
| GCC -O0   |          509 |         3095 |       3604 |
| GCC -O2   |          817 |         1760 |       2577 |
| Clang -O0 |          452 |         3074 |       3526 |
| Clang -O2 |          813 |         1742 |       2555 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    694 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    124 us
  link        bench_rcc     :    133 us
  link        bench_rcc     :  51865 us

RCC -O1:
  preprocess  bench.c       :    806 us
  parse       bench.c       :    169 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    160 us
  link        bench_o1      :    119 us
  link        bench_o1      :  49511 us

RCC -O2:
  preprocess  bench.c       :    558 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    140 us
  link        bench_o2      :  50079 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 248178 us
  parse       sqlite3.c     :  50246 us
  typecheck   sqlite3.c     :  11005 us
  codegen     sqlite3.c     : 113935 us
  link        sqlite3.so    :  16035 us

RCC -O1:
  preprocess  sqlite3.c     : 272681 us
  parse       sqlite3.c     :  72681 us
  typecheck   sqlite3.c     :  13430 us
  opt         sqlite3.c     : 153785 us
  codegen     sqlite3.c     : 136545 us
  link        sqlite3.so    :  20353 us

RCC -O2:
  preprocess  sqlite3.c     : 317592 us
  parse       sqlite3.c     :  61868 us
  typecheck   sqlite3.c     :  19287 us
  opt         sqlite3.c     : 180631 us
  codegen     sqlite3.c     : 157492 us
  link        sqlite3.so    :  21530 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       887 ms |
| RCC -O1   |       668 ms |
| RCC -O2   |       650 ms |
| TCC       |        99 ms |
| GCC -O0   |       936 ms |
| GCC -O2   |      8661 ms |
| Clang -O0 |       908 ms |
| Clang -O2 |      8684 ms |
