# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           66 |          674 |        740 |
| RCC -O1   |           74 |          691 |        765 |
| RCC -O2   |           63 |          694 |        757 |
| TCC       |           52 |          648 |        700 |
| GCC -O0   |           98 |          499 |        597 |
| GCC -O2   |          106 |          294 |        400 |
| Clang -O0 |           81 |          511 |        592 |
| Clang -O2 |          140 |          298 |        438 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    956 us
  parse       bench.c       :    162 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    215 us
  link        bench_rcc     :    259 us
  link        bench_rcc     :  60072 us

RCC -O1:
  preprocess  bench.c       :    678 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    152 us
  link        bench_o1      :    313 us
  link        bench_o1      :  72971 us

RCC -O2:
  preprocess  bench.c       :    818 us
  parse       bench.c       :    181 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    166 us
  link        bench_o2      :    161 us
  link        bench_o2      :  74173 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 367727 us
  parse       sqlite3.c     :  79976 us
  typecheck   sqlite3.c     :  17912 us
  codegen     sqlite3.c     : 120256 us
  link        sqlite3.so    :  18703 us

RCC -O1:
  preprocess  sqlite3.c     : 271140 us
  parse       sqlite3.c     :  59023 us
  typecheck   sqlite3.c     :  14460 us
  opt         sqlite3.c     : 155904 us
  codegen     sqlite3.c     : 114021 us
  link        sqlite3.so    :  20246 us

RCC -O2:
  preprocess  sqlite3.c     : 246978 us
  parse       sqlite3.c     :  50858 us
  typecheck   sqlite3.c     :  13206 us
  opt         sqlite3.c     : 157456 us
  codegen     sqlite3.c     : 112062 us
  link        sqlite3.so    :  17451 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       733 ms |
| RCC -O1   |       982 ms |
| RCC -O2   |       926 ms |
| TCC       |       124 ms |
| GCC -O0   |      1359 ms |
| GCC -O2   |     13835 ms |
| Clang -O0 |      1657 ms |
| Clang -O2 |     13039 ms |
