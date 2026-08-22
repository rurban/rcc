# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          616 |        676 |
| RCC -O1   |           53 |          613 |        666 |
| RCC -O2   |           58 |          622 |        680 |
| TCC       |           44 |          534 |        578 |
| GCC -O0   |           64 |          450 |        514 |
| GCC -O2   |          103 |          272 |        375 |
| Clang -O0 |           55 |          449 |        504 |
| Clang -O2 |           85 |          274 |        359 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    651 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    128 us
  link        bench_rcc     :    110 us
  link        bench_rcc     :  46059 us

RCC -O1:
  preprocess  bench.c       :    626 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    111 us
  link        bench_o1      :  47695 us

RCC -O2:
  preprocess  bench.c       :    612 us
  parse       bench.c       :    131 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    127 us
  link        bench_o2      :    155 us
  link        bench_o2      :  47943 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 186331 us
  parse       sqlite3.c     :  48078 us
  typecheck   sqlite3.c     :  11536 us
  codegen     sqlite3.c     :  87022 us
  link        sqlite3.so    :  15017 us

RCC -O1:
  preprocess  sqlite3.c     : 184517 us
  parse       sqlite3.c     :  47921 us
  typecheck   sqlite3.c     :  13203 us
  opt         sqlite3.c     : 124481 us
  codegen     sqlite3.c     :  88265 us
  link        sqlite3.so    :  16196 us

RCC -O2:
  preprocess  sqlite3.c     : 185106 us
  parse       sqlite3.c     :  45391 us
  typecheck   sqlite3.c     :  12086 us
  opt         sqlite3.c     : 131916 us
  codegen     sqlite3.c     :  91027 us
  link        sqlite3.so    :  16160 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       600 ms |
| RCC -O1   |       665 ms |
| RCC -O2   |       666 ms |
| TCC       |        94 ms |
| GCC -O0   |       970 ms |
| GCC -O2   |      9548 ms |
| Clang -O0 |       959 ms |
| Clang -O2 |      9171 ms |
