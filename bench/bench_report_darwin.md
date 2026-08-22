# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          125 |          789 |        914 |
| RCC -O1   |          105 |          756 |        861 |
| RCC -O2   |           93 |          763 |        856 |
| TCC       |           74 |          614 |        688 |
| GCC -O0   |          129 |          618 |        747 |
| GCC -O2   |          157 |          335 |        492 |
| Clang -O0 |           78 |          656 |        734 |
| Clang -O2 |          153 |          364 |        517 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2545 us
  parse       bench.c       :    205 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    144 us
  link        bench_rcc     :    368 us
  link        bench_rcc     :  93874 us

RCC -O1:
  preprocess  bench.c       :    682 us
  parse       bench.c       :    197 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    146 us
  link        bench_o1      :    642 us
  link        bench_o1      : 108953 us

RCC -O2:
  preprocess  bench.c       :   1189 us
  parse       bench.c       :    166 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    133 us
  link        bench_o2      :    614 us
  link        bench_o2      :  90672 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 515863 us
  parse       sqlite3.c     : 247625 us
  typecheck   sqlite3.c     :  37560 us
  codegen     sqlite3.c     : 249260 us
  link        sqlite3.so    :  20519 us

RCC -O1:
  preprocess  sqlite3.c     : 417315 us
  parse       sqlite3.c     : 107987 us
  typecheck   sqlite3.c     :  37083 us
  opt         sqlite3.c     : 252985 us
  codegen     sqlite3.c     : 234277 us
  link        sqlite3.so    :  26900 us

RCC -O2:
  preprocess  sqlite3.c     : 420826 us
  parse       sqlite3.c     :  93638 us
  typecheck   sqlite3.c     :  22301 us
  opt         sqlite3.c     : 234743 us
  codegen     sqlite3.c     : 171300 us
  link        sqlite3.so    :  26906 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1658 ms |
| RCC -O1   |      1527 ms |
| RCC -O2   |      1436 ms |
| TCC       |       161 ms |
| GCC -O0   |      2277 ms |
| GCC -O2   |     17605 ms |
| Clang -O0 |      1402 ms |
| Clang -O2 |     17436 ms |
