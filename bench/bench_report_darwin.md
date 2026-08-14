# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          701 |        767 |
| RCC -O1   |          103 |          756 |        859 |
| RCC -O2   |           83 |          888 |        971 |
| TCC       |          215 |          723 |        938 |
| GCC -O0   |          162 |          603 |        765 |
| GCC -O2   |          148 |          354 |        502 |
| Clang -O0 |           93 |          597 |        690 |
| Clang -O2 |          155 |          353 |        508 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    871 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    147 us
  link        bench_rcc     :    506 us
  link        bench_rcc     :  53105 us

RCC -O1:
  preprocess  bench.c       :    645 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    113 us
  link        bench_o1      :  51516 us

RCC -O2:
  preprocess  bench.c       :    657 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     30 us
  codegen     bench.c       :    136 us
  link        bench_o2      :    126 us
  link        bench_o2      :  45329 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 313982 us
  parse       sqlite3.c     : 100152 us
  typecheck   sqlite3.c     :  24874 us
  codegen     sqlite3.c     : 114353 us
  link        sqlite3.so    :  15667 us

RCC -O1:
  preprocess  sqlite3.c     : 298908 us
  parse       sqlite3.c     :  99217 us
  typecheck   sqlite3.c     :  13247 us
  opt         sqlite3.c     : 132360 us
  codegen     sqlite3.c     : 109490 us
  link        sqlite3.so    :  15039 us

RCC -O2:
  preprocess  sqlite3.c     : 239473 us
  parse       sqlite3.c     :  50099 us
  typecheck   sqlite3.c     :  21135 us
  opt         sqlite3.c     : 238180 us
  codegen     sqlite3.c     : 134791 us
  link        sqlite3.so    :  17888 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1326 ms |
| RCC -O1   |      1125 ms |
| RCC -O2   |      1331 ms |
| TCC       |       308 ms |
| GCC -O0   |      1584 ms |
| GCC -O2   |     14658 ms |
| Clang -O0 |      1851 ms |
| Clang -O2 |     16198 ms |
