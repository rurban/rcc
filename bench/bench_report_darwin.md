# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          620 |        673 |
| RCC -O1   |           53 |          607 |        660 |
| RCC -O2   |           55 |          604 |        659 |
| TCC       |           50 |          522 |        572 |
| GCC -O0   |           69 |          448 |        517 |
| GCC -O2   |          108 |          266 |        374 |
| Clang -O0 |           54 |          441 |        495 |
| Clang -O2 |          132 |          265 |        397 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    873 us
  parse       bench.c       :    174 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    168 us
  link        bench_rcc     :    219 us
  link        bench_rcc     :  52681 us

RCC -O1:
  preprocess  bench.c       :    612 us
  parse       bench.c       :    134 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    114 us
  link        bench_o1      :    144 us
  link        bench_o1      :  48062 us

RCC -O2:
  preprocess  bench.c       :    599 us
  parse       bench.c       :    127 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    134 us
  link        bench_o2      :    116 us
  link        bench_o2      :  53349 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 251618 us
  parse       sqlite3.c     :  51485 us
  typecheck   sqlite3.c     :  12045 us
  codegen     sqlite3.c     :  91856 us
  link        sqlite3.so    :  15058 us

RCC -O1:
  preprocess  sqlite3.c     : 212017 us
  parse       sqlite3.c     :  43868 us
  typecheck   sqlite3.c     :  12028 us
  opt         sqlite3.c     :  18829 us
  codegen     sqlite3.c     :  90984 us
  link        sqlite3.so    :  15057 us

RCC -O2:
  preprocess  sqlite3.c     : 198654 us
  parse       sqlite3.c     :  43091 us
  typecheck   sqlite3.c     :  12041 us
  opt         sqlite3.c     : 134726 us
  codegen     sqlite3.c     :  92076 us
  link        sqlite3.so    :  15809 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       598 ms |
| RCC -O1   |       607 ms |
| RCC -O2   |       842 ms |
| TCC       |        92 ms |
| GCC -O0   |      1035 ms |
| GCC -O2   |     11666 ms |
| Clang -O0 |      1266 ms |
| Clang -O2 |     12060 ms |
