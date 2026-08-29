# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           47 |          591 |        638 |
| RCC -O1   |           47 |          599 |        646 |
| RCC -O2   |           55 |          601 |        656 |
| TCC       |           42 |          521 |        563 |
| GCC -O0   |           68 |          435 |        503 |
| GCC -O2   |          102 |          263 |        365 |
| Clang -O0 |           55 |          435 |        490 |
| Clang -O2 |           92 |          263 |        355 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    870 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    170 us
  link        bench_rcc     :     94 us
  link        bench_rcc     :  71044 us

RCC -O1:
  preprocess  bench.c       :    597 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    146 us
  link        bench_o1      :     97 us
  link        bench_o1      :  47282 us

RCC -O2:
  preprocess  bench.c       :    786 us
  parse       bench.c       :    189 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    133 us
  link        bench_o2      :    195 us
  link        bench_o2      :  61684 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 233126 us
  parse       sqlite3.c     :  50534 us
  typecheck   sqlite3.c     :  10223 us
  codegen     sqlite3.c     :  88082 us
  link        sqlite3.so    :  13976 us

RCC -O1:
  preprocess  sqlite3.c     : 173547 us
  parse       sqlite3.c     :  44892 us
  typecheck   sqlite3.c     :  10018 us
  opt         sqlite3.c     : 123867 us
  codegen     sqlite3.c     :  88921 us
  link        sqlite3.so    :  16043 us

RCC -O2:
  preprocess  sqlite3.c     : 211677 us
  parse       sqlite3.c     :  52764 us
  typecheck   sqlite3.c     :  11242 us
  opt         sqlite3.c     : 153363 us
  codegen     sqlite3.c     :  99199 us
  link        sqlite3.so    :  13605 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       656 ms |
| RCC -O1   |       643 ms |
| RCC -O2   |       651 ms |
| TCC       |        93 ms |
| GCC -O0   |       914 ms |
| GCC -O2   |      9237 ms |
| Clang -O0 |      1143 ms |
| Clang -O2 |      8759 ms |
