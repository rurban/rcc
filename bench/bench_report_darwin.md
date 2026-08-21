# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          705 |        792 |
| RCC -O1   |           69 |          717 |        786 |
| RCC -O2   |           74 |          696 |        770 |
| TCC       |           55 |          588 |        643 |
| GCC -O0   |           96 |          528 |        624 |
| GCC -O2   |          140 |          307 |        447 |
| Clang -O0 |           79 |          500 |        579 |
| Clang -O2 |           95 |          293 |        388 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    780 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    180 us
  link        bench_rcc     :    128 us
  link        bench_rcc     :  56687 us

RCC -O1:
  preprocess  bench.c       :    746 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :     42 us
  opt         bench.c       :     58 us
  codegen     bench.c       :    158 us
  link        bench_o1      :    163 us
  link        bench_o1      :  60544 us

RCC -O2:
  preprocess  bench.c       :    871 us
  parse       bench.c       :    193 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    151 us
  link        bench_o2      :    178 us
  link        bench_o2      :  72485 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 228243 us
  parse       sqlite3.c     :  61257 us
  typecheck   sqlite3.c     :  12496 us
  codegen     sqlite3.c     : 153651 us
  link        sqlite3.so    :  14627 us

RCC -O1:
  preprocess  sqlite3.c     : 270008 us
  parse       sqlite3.c     :  61235 us
  typecheck   sqlite3.c     :  12658 us
  opt         sqlite3.c     : 133631 us
  codegen     sqlite3.c     : 155661 us
  link        sqlite3.so    :  21136 us

RCC -O2:
  preprocess  sqlite3.c     : 263933 us
  parse       sqlite3.c     :  73643 us
  typecheck   sqlite3.c     :  15449 us
  opt         sqlite3.c     : 192942 us
  codegen     sqlite3.c     : 104878 us
  link        sqlite3.so    :  16194 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       688 ms |
| RCC -O1   |       843 ms |
| RCC -O2   |       918 ms |
| TCC       |       104 ms |
| GCC -O0   |      1147 ms |
| GCC -O2   |     11879 ms |
| Clang -O0 |      1575 ms |
| Clang -O2 |     11689 ms |
