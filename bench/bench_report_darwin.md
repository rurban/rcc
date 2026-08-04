# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          781 |        878 |
| RCC -O1   |           78 |          767 |        845 |
| RCC -O2   |           76 |          797 |        873 |
| TCC       |           70 |          734 |        804 |
| GCC -O0   |          123 |          615 |        738 |
| GCC -O2   |          221 |          354 |        575 |
| Clang -O0 |           89 |          582 |        671 |
| Clang -O2 |          121 |          326 |        447 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1566 us
  parse       bench.c       :    294 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    270 us
  link        bench_rcc     :    243 us
  link        bench_rcc     :  76767 us

RCC -O1:
  preprocess  bench.c       :    656 us
  parse       bench.c       :    120 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    282 us
  link        bench_o1      :  69238 us

RCC -O2:
  preprocess  bench.c       :    602 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     15 us
  codegen     bench.c       :    104 us
  link        bench_o2      :    232 us
  link        bench_o2      :  66809 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 299218 us
  parse       sqlite3.c     :  72504 us
  typecheck   sqlite3.c     :  20646 us
  codegen     sqlite3.c     : 154555 us
  link        sqlite3.so    :  21941 us

RCC -O1:
  preprocess  sqlite3.c     : 343141 us
  parse       sqlite3.c     :  61804 us
  typecheck   sqlite3.c     :  16539 us
  opt         sqlite3.c     :  28984 us
  codegen     sqlite3.c     : 151143 us
  link        sqlite3.so    :  20170 us

RCC -O2:
  preprocess  sqlite3.c     : 300899 us
  parse       sqlite3.c     :  67889 us
  typecheck   sqlite3.c     :  20201 us
  opt         sqlite3.c     : 199500 us
  codegen     sqlite3.c     : 127779 us
  link        sqlite3.so    :  21048 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1501 ms |
| RCC -O1   |       902 ms |
| RCC -O2   |       969 ms |
| TCC       |       139 ms |
| GCC -O0   |      1491 ms |
| GCC -O2   |     14594 ms |
| Clang -O0 |      1394 ms |
| Clang -O2 |     13449 ms |
