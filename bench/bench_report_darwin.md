# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          689 |        743 |
| RCC -O1   |           71 |          645 |        716 |
| RCC -O2   |           63 |          639 |        702 |
| TCC       |           63 |          555 |        618 |
| GCC -O0   |           76 |          467 |        543 |
| GCC -O2   |          104 |          283 |        387 |
| Clang -O0 |           65 |          468 |        533 |
| Clang -O2 |           98 |          282 |        380 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    737 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    142 us
  link        bench_rcc     :    112 us
  link        bench_rcc     :  46599 us

RCC -O1:
  preprocess  bench.c       :    683 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    158 us
  link        bench_o1      :    118 us
  link        bench_o1      :  53009 us

RCC -O2:
  preprocess  bench.c       :    655 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    128 us
  link        bench_o2      :    132 us
  link        bench_o2      :  54120 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 244673 us
  parse       sqlite3.c     :  53714 us
  typecheck   sqlite3.c     :  13707 us
  codegen     sqlite3.c     :  98091 us
  link        sqlite3.so    :  15024 us

RCC -O1:
  preprocess  sqlite3.c     : 201042 us
  parse       sqlite3.c     :  49303 us
  typecheck   sqlite3.c     :  12549 us
  opt         sqlite3.c     : 128537 us
  codegen     sqlite3.c     :  95477 us
  link        sqlite3.so    :  15780 us

RCC -O2:
  preprocess  sqlite3.c     : 198487 us
  parse       sqlite3.c     :  48216 us
  typecheck   sqlite3.c     :  12349 us
  opt         sqlite3.c     : 156032 us
  codegen     sqlite3.c     :  96070 us
  link        sqlite3.so    :  15879 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       671 ms |
| RCC -O1   |       733 ms |
| RCC -O2   |       768 ms |
| TCC       |        97 ms |
| GCC -O0   |      1037 ms |
| GCC -O2   |     10217 ms |
| Clang -O0 |      1150 ms |
| Clang -O2 |      9943 ms |
