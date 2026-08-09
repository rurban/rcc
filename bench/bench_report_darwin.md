# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           85 |          875 |        960 |
| RCC -O1   |          155 |          874 |       1029 |
| RCC -O2   |          119 |          882 |       1001 |
| TCC       |           85 |          760 |        845 |
| GCC -O0   |          166 |          776 |        942 |
| GCC -O2   |          268 |          372 |        640 |
| Clang -O0 |          108 |          699 |        807 |
| Clang -O2 |          219 |          346 |        565 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    866 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    163 us
  link        bench_rcc     :    387 us
  link        bench_rcc     :  65123 us

RCC -O1:
  preprocess  bench.c       :    632 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    109 us
  link        bench_o1      :    463 us
  link        bench_o1      :  64733 us

RCC -O2:
  preprocess  bench.c       :    638 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    112 us
  link        bench_o2      :    173 us
  link        bench_o2      :  64533 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 339208 us
  parse       sqlite3.c     : 122464 us
  typecheck   sqlite3.c     :  17859 us
  codegen     sqlite3.c     : 118078 us
  link        sqlite3.so    :  21498 us

RCC -O1:
  preprocess  sqlite3.c     : 276441 us
  parse       sqlite3.c     :  53170 us
  typecheck   sqlite3.c     :  14219 us
  opt         sqlite3.c     :  30112 us
  codegen     sqlite3.c     : 112323 us
  link        sqlite3.so    :  19212 us

RCC -O2:
  preprocess  sqlite3.c     : 263365 us
  parse       sqlite3.c     :  64715 us
  typecheck   sqlite3.c     :  17216 us
  opt         sqlite3.c     : 159705 us
  codegen     sqlite3.c     : 167275 us
  link        sqlite3.so    :  18694 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1072 ms |
| RCC -O1   |       972 ms |
| RCC -O2   |      1140 ms |
| TCC       |       224 ms |
| GCC -O0   |      1297 ms |
| GCC -O2   |     14969 ms |
| Clang -O0 |      1550 ms |
| Clang -O2 |     13558 ms |
