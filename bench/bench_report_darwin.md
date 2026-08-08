# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          697 |        761 |
| RCC -O1   |          113 |          867 |        980 |
| RCC -O2   |          103 |          781 |        884 |
| TCC       |           68 |          690 |        758 |
| GCC -O0   |          139 |          492 |        631 |
| GCC -O2   |          126 |          297 |        423 |
| Clang -O0 |           83 |          510 |        593 |
| Clang -O2 |          125 |          295 |        420 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    828 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    218 us
  link        bench_rcc     :    152 us
  link        bench_rcc     :  85804 us

RCC -O1:
  preprocess  bench.c       :    843 us
  parse       bench.c       :    259 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    222 us
  link        bench_o1      :    189 us
  link        bench_o1      :  91939 us

RCC -O2:
  preprocess  bench.c       :   2177 us
  parse       bench.c       :    259 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     30 us
  codegen     bench.c       :    188 us
  link        bench_o2      :    164 us
  link        bench_o2      :  73900 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 342533 us
  parse       sqlite3.c     :  64176 us
  typecheck   sqlite3.c     :  15524 us
  codegen     sqlite3.c     : 136710 us
  link        sqlite3.so    :  16326 us

RCC -O1:
  preprocess  sqlite3.c     : 281593 us
  parse       sqlite3.c     :  53222 us
  typecheck   sqlite3.c     :  14746 us
  opt         sqlite3.c     :  22380 us
  codegen     sqlite3.c     : 176091 us
  link        sqlite3.so    :  17649 us

RCC -O2:
  preprocess  sqlite3.c     : 324954 us
  parse       sqlite3.c     :  59772 us
  typecheck   sqlite3.c     :  17396 us
  opt         sqlite3.c     : 198908 us
  codegen     sqlite3.c     : 121206 us
  link        sqlite3.so    :  18290 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1098 ms |
| RCC -O1   |       684 ms |
| RCC -O2   |       798 ms |
| TCC       |       124 ms |
| GCC -O0   |      1055 ms |
| GCC -O2   |     12702 ms |
| Clang -O0 |      1537 ms |
| Clang -O2 |     11165 ms |
