# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          663 |        737 |
| RCC -O1   |           61 |          661 |        722 |
| RCC -O2   |           52 |          633 |        685 |
| TCC       |           61 |          539 |        600 |
| GCC -O0   |           80 |          466 |        546 |
| GCC -O2   |          110 |          291 |        401 |
| Clang -O0 |           78 |          483 |        561 |
| Clang -O2 |          108 |          300 |        408 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    592 us
  parse       bench.c       :    118 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    114 us
  link        bench_rcc     :    210 us
  link        bench_rcc     :  43702 us

RCC -O1:
  preprocess  bench.c       :    560 us
  parse       bench.c       :    125 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    115 us
  link        bench_o1      :    140 us
  link        bench_o1      :  43878 us

RCC -O2:
  preprocess  bench.c       :    528 us
  parse       bench.c       :    113 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    110 us
  link        bench_o2      :    557 us
  link        bench_o2      :  41874 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 219225 us
  parse       sqlite3.c     : 239337 us
  typecheck   sqlite3.c     :  21621 us
  codegen     sqlite3.c     : 122383 us
  link        sqlite3.so    :  15502 us

RCC -O1:
  preprocess  sqlite3.c     : 542270 us
  parse       sqlite3.c     :  77657 us
  typecheck   sqlite3.c     :  32595 us
  opt         sqlite3.c     : 198908 us
  codegen     sqlite3.c     : 154594 us
  link        sqlite3.so    :  17324 us

RCC -O2:
  preprocess  sqlite3.c     : 267031 us
  parse       sqlite3.c     :  53350 us
  typecheck   sqlite3.c     :  13486 us
  opt         sqlite3.c     : 152088 us
  codegen     sqlite3.c     : 103668 us
  link        sqlite3.so    :  15946 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1126 ms |
| RCC -O1   |       918 ms |
| RCC -O2   |       795 ms |
| TCC       |       114 ms |
| GCC -O0   |      1103 ms |
| GCC -O2   |     12803 ms |
| Clang -O0 |      1520 ms |
| Clang -O2 |     12964 ms |
