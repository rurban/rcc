# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          120 |          726 |        846 |
| RCC -O1   |          146 |          699 |        845 |
| RCC -O2   |           68 |          677 |        745 |
| TCC       |          120 |          621 |        741 |
| GCC -O0   |          119 |          526 |        645 |
| GCC -O2   |          135 |          311 |        446 |
| Clang -O0 |           68 |          514 |        582 |
| Clang -O2 |          131 |          300 |        431 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    833 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    141 us
  link        bench_rcc     :    431 us
  link        bench_rcc     :  91129 us

RCC -O1:
  preprocess  bench.c       :   1350 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    184 us
  link        bench_o1      :    303 us
  link        bench_o1      :  88195 us

RCC -O2:
  preprocess  bench.c       :   1672 us
  parse       bench.c       :    132 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    138 us
  link        bench_o2      :    460 us
  link        bench_o2      :  83472 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 471672 us
  parse       sqlite3.c     : 224146 us
  typecheck   sqlite3.c     :  24203 us
  codegen     sqlite3.c     : 199787 us
  link        sqlite3.so    :  16462 us

RCC -O1:
  preprocess  sqlite3.c     : 444728 us
  parse       sqlite3.c     :  62590 us
  typecheck   sqlite3.c     :  20521 us
  opt         sqlite3.c     : 271742 us
  codegen     sqlite3.c     : 118205 us
  link        sqlite3.so    :  19609 us

RCC -O2:
  preprocess  sqlite3.c     : 402376 us
  parse       sqlite3.c     :  70015 us
  typecheck   sqlite3.c     :  14342 us
  opt         sqlite3.c     : 254287 us
  codegen     sqlite3.c     : 312618 us
  link        sqlite3.so    :  24043 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1068 ms |
| RCC -O1   |      1387 ms |
| RCC -O2   |      1177 ms |
| TCC       |       182 ms |
| GCC -O0   |      1644 ms |
| GCC -O2   |     14654 ms |
| Clang -O0 |      1310 ms |
| Clang -O2 |     12154 ms |
