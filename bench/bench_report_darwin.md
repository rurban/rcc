# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          164 |          736 |        900 |
| RCC -O1   |           75 |          681 |        756 |
| RCC -O2   |           80 |          646 |        726 |
| TCC       |           46 |          553 |        599 |
| GCC -O0   |           89 |          451 |        540 |
| GCC -O2   |          106 |          282 |        388 |
| Clang -O0 |           61 |          476 |        537 |
| Clang -O2 |          109 |          333 |        442 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    829 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    165 us
  link        bench_rcc     :    572 us
  link        bench_rcc     :  66255 us

RCC -O1:
  preprocess  bench.c       :    728 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    345 us
  link        bench_o1      :    618 us
  link        bench_o1      :  67036 us

RCC -O2:
  preprocess  bench.c       :    613 us
  parse       bench.c       :    220 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    129 us
  link        bench_o2      :     81 us
  link        bench_o2      :  68098 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 399687 us
  parse       sqlite3.c     : 217650 us
  typecheck   sqlite3.c     :  25910 us
  codegen     sqlite3.c     : 175683 us
  link        sqlite3.so    :  56754 us

RCC -O1:
  preprocess  sqlite3.c     : 358892 us
  parse       sqlite3.c     :  65123 us
  typecheck   sqlite3.c     :  27921 us
  opt         sqlite3.c     :  36492 us
  codegen     sqlite3.c     : 151834 us
  link        sqlite3.so    :  18588 us

RCC -O2:
  preprocess  sqlite3.c     : 388040 us
  parse       sqlite3.c     :  57309 us
  typecheck   sqlite3.c     :  15384 us
  opt         sqlite3.c     : 233022 us
  codegen     sqlite3.c     : 159834 us
  link        sqlite3.so    :  18099 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       901 ms |
| RCC -O1   |       921 ms |
| RCC -O2   |       952 ms |
| TCC       |       127 ms |
| GCC -O0   |      1715 ms |
| GCC -O2   |     18483 ms |
| Clang -O0 |      2733 ms |
| Clang -O2 |     19475 ms |
