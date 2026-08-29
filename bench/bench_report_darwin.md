# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           83 |          719 |        802 |
| RCC -O1   |           82 |          759 |        841 |
| RCC -O2   |           69 |          736 |        805 |
| TCC       |           61 |          623 |        684 |
| GCC -O0   |           85 |          581 |        666 |
| GCC -O2   |          180 |          324 |        504 |
| Clang -O0 |           79 |          525 |        604 |
| Clang -O2 |          122 |          302 |        424 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    995 us
  parse       bench.c       :    236 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    197 us
  link        bench_rcc     :    634 us
  link        bench_rcc     :  77932 us

RCC -O1:
  preprocess  bench.c       :    764 us
  parse       bench.c       :    236 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    145 us
  link        bench_o1      :    580 us
  link        bench_o1      :  68737 us

RCC -O2:
  preprocess  bench.c       :    754 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    176 us
  link        bench_o2      :    322 us
  link        bench_o2      :  82445 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 295928 us
  parse       sqlite3.c     :  65424 us
  typecheck   sqlite3.c     :  15255 us
  codegen     sqlite3.c     : 124822 us
  link        sqlite3.so    :  18916 us

RCC -O1:
  preprocess  sqlite3.c     : 253984 us
  parse       sqlite3.c     :  66782 us
  typecheck   sqlite3.c     :  17823 us
  opt         sqlite3.c     : 191614 us
  codegen     sqlite3.c     : 135415 us
  link        sqlite3.so    :  19782 us

RCC -O2:
  preprocess  sqlite3.c     : 246976 us
  parse       sqlite3.c     :  65791 us
  typecheck   sqlite3.c     :  16752 us
  opt         sqlite3.c     : 244396 us
  codegen     sqlite3.c     : 149141 us
  link        sqlite3.so    :  20280 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       749 ms |
| RCC -O1   |       892 ms |
| RCC -O2   |       934 ms |
| TCC       |       144 ms |
| GCC -O0   |      1507 ms |
| GCC -O2   |     13949 ms |
| Clang -O0 |      1523 ms |
| Clang -O2 |     13524 ms |
