# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          106 |          782 |        888 |
| RCC -O1   |           93 |          685 |        778 |
| RCC -O2   |          129 |          710 |        839 |
| TCC       |          107 |          670 |        777 |
| GCC -O0   |          138 |          518 |        656 |
| GCC -O2   |          132 |          317 |        449 |
| Clang -O0 |           95 |          520 |        615 |
| Clang -O2 |          133 |          304 |        437 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1697 us
  parse       bench.c       :    240 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    166 us
  link        bench_rcc     :    140 us
  link        bench_rcc     :  67346 us

RCC -O1:
  preprocess  bench.c       :    736 us
  parse       bench.c       :    223 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    142 us
  link        bench_o1      :    222 us
  link        bench_o1      :  66774 us

RCC -O2:
  preprocess  bench.c       :    714 us
  parse       bench.c       :    147 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    149 us
  link        bench_o2      :    140 us
  link        bench_o2      :  77974 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 383999 us
  parse       sqlite3.c     :  89882 us
  typecheck   sqlite3.c     :  14225 us
  codegen     sqlite3.c     : 152211 us
  link        sqlite3.so    :  19978 us

RCC -O1:
  preprocess  sqlite3.c     : 370347 us
  parse       sqlite3.c     :  82993 us
  typecheck   sqlite3.c     :  16210 us
  opt         sqlite3.c     : 184889 us
  codegen     sqlite3.c     : 131922 us
  link        sqlite3.so    :  20677 us

RCC -O2:
  preprocess  sqlite3.c     : 303554 us
  parse       sqlite3.c     :  68105 us
  typecheck   sqlite3.c     :  15358 us
  opt         sqlite3.c     : 173908 us
  codegen     sqlite3.c     : 115339 us
  link        sqlite3.so    :  21360 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       858 ms |
| RCC -O1   |       993 ms |
| RCC -O2   |      1092 ms |
| TCC       |       167 ms |
| GCC -O0   |      1412 ms |
| GCC -O2   |     14338 ms |
| Clang -O0 |      1364 ms |
| Clang -O2 |     13165 ms |
