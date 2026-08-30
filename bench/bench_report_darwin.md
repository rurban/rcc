# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           68 |          664 |        732 |
| RCC -O1   |           53 |          659 |        712 |
| RCC -O2   |           57 |          656 |        713 |
| TCC       |           45 |          550 |        595 |
| GCC -O0   |           78 |          463 |        541 |
| GCC -O2   |          127 |          286 |        413 |
| Clang -O0 |           77 |          459 |        536 |
| Clang -O2 |           87 |          263 |        350 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    930 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    137 us
  link        bench_rcc     :    101 us
  link        bench_rcc     :  44416 us

RCC -O1:
  preprocess  bench.c       :    554 us
  parse       bench.c       :    115 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    114 us
  link        bench_o1      :    307 us
  link        bench_o1      :  43745 us

RCC -O2:
  preprocess  bench.c       :    602 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    114 us
  link        bench_o2      :    154 us
  link        bench_o2      :  44302 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 250627 us
  parse       sqlite3.c     : 113354 us
  typecheck   sqlite3.c     :  21322 us
  codegen     sqlite3.c     : 118764 us
  link        sqlite3.so    :  18242 us

RCC -O1:
  preprocess  sqlite3.c     : 211674 us
  parse       sqlite3.c     :  59634 us
  typecheck   sqlite3.c     :  11841 us
  opt         sqlite3.c     : 131614 us
  codegen     sqlite3.c     : 111675 us
  link        sqlite3.so    :  18245 us

RCC -O2:
  preprocess  sqlite3.c     : 252742 us
  parse       sqlite3.c     :  54706 us
  typecheck   sqlite3.c     :  13790 us
  opt         sqlite3.c     : 163846 us
  codegen     sqlite3.c     : 121727 us
  link        sqlite3.so    :  15830 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       746 ms |
| RCC -O1   |       641 ms |
| RCC -O2   |       648 ms |
| TCC       |       100 ms |
| GCC -O0   |      1031 ms |
| GCC -O2   |      9262 ms |
| Clang -O0 |      1013 ms |
| Clang -O2 |      9308 ms |
