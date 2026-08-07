# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           95 |          712 |        807 |
| RCC -O1   |          109 |          637 |        746 |
| RCC -O2   |           88 |          763 |        851 |
| TCC       |           82 |          611 |        693 |
| GCC -O0   |          110 |          578 |        688 |
| GCC -O2   |          207 |          292 |        499 |
| Clang -O0 |           63 |          480 |        543 |
| Clang -O2 |           93 |          301 |        394 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    951 us
  parse       bench.c       :    222 us
  typecheck   bench.c       :     23 us
  codegen     bench.c       :    326 us
  link        bench_rcc     :    439 us
  link        bench_rcc     : 106605 us

RCC -O1:
  preprocess  bench.c       :    785 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    337 us
  link        bench_o1      :    232 us
  link        bench_o1      :  94248 us

RCC -O2:
  preprocess  bench.c       :    705 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    148 us
  link        bench_o2      :    230 us
  link        bench_o2      :  86639 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 782599 us
  parse       sqlite3.c     : 103750 us
  typecheck   sqlite3.c     :  24179 us
  codegen     sqlite3.c     : 182755 us
  link        sqlite3.so    :  23541 us

RCC -O1:
  preprocess  sqlite3.c     : 329753 us
  parse       sqlite3.c     :  50151 us
  typecheck   sqlite3.c     :  13957 us
  opt         sqlite3.c     :  23393 us
  codegen     sqlite3.c     : 166137 us
  link        sqlite3.so    :  30891 us

RCC -O2:
  preprocess  sqlite3.c     : 381095 us
  parse       sqlite3.c     :  79045 us
  typecheck   sqlite3.c     :  27082 us
  opt         sqlite3.c     : 250634 us
  codegen     sqlite3.c     : 117828 us
  link        sqlite3.so    :  15623 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       926 ms |
| RCC -O1   |       651 ms |
| RCC -O2   |       753 ms |
| TCC       |       136 ms |
| GCC -O0   |      1392 ms |
| GCC -O2   |     11821 ms |
| Clang -O0 |      1061 ms |
| Clang -O2 |     10202 ms |
