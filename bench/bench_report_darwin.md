# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           75 |          667 |        742 |
| RCC -O1   |           64 |          672 |        736 |
| RCC -O2   |           58 |          668 |        726 |
| TCC       |           49 |          602 |        651 |
| GCC -O0   |           93 |          509 |        602 |
| GCC -O2   |          125 |          303 |        428 |
| Clang -O0 |          109 |          473 |        582 |
| Clang -O2 |          131 |          312 |        443 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    851 us
  parse       bench.c       :    205 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    181 us
  link        bench_rcc     :    489 us
  link        bench_rcc     :  55493 us

RCC -O1:
  preprocess  bench.c       :    733 us
  parse       bench.c       :    186 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    176 us
  link        bench_o1      :    502 us
  link        bench_o1      :  62817 us

RCC -O2:
  preprocess  bench.c       :    776 us
  parse       bench.c       :    190 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    170 us
  link        bench_o2      :    404 us
  link        bench_o2      :  49728 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 239933 us
  parse       sqlite3.c     :  62340 us
  typecheck   sqlite3.c     :  17617 us
  codegen     sqlite3.c     : 100261 us
  link        sqlite3.so    :  15837 us

RCC -O1:
  preprocess  sqlite3.c     : 224389 us
  parse       sqlite3.c     :  55951 us
  typecheck   sqlite3.c     :  17183 us
  opt         sqlite3.c     : 160023 us
  codegen     sqlite3.c     : 101149 us
  link        sqlite3.so    :  15452 us

RCC -O2:
  preprocess  sqlite3.c     : 238744 us
  parse       sqlite3.c     :  51486 us
  typecheck   sqlite3.c     :  12579 us
  opt         sqlite3.c     : 182052 us
  codegen     sqlite3.c     : 110776 us
  link        sqlite3.so    :  15754 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1192 ms |
| RCC -O1   |      1234 ms |
| RCC -O2   |       968 ms |
| TCC       |       167 ms |
| GCC -O0   |      1466 ms |
| GCC -O2   |     13560 ms |
| Clang -O0 |      1176 ms |
| Clang -O2 |     11481 ms |
