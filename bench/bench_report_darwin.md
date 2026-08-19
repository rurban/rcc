# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           61 |          636 |        697 |
| RCC -O1   |           65 |          637 |        702 |
| RCC -O2   |           59 |          635 |        694 |
| TCC       |           46 |          573 |        619 |
| GCC -O0   |           81 |          524 |        605 |
| GCC -O2   |          123 |          295 |        418 |
| Clang -O0 |           80 |          501 |        581 |
| Clang -O2 |          102 |          280 |        382 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    152 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    158 us
  link        bench_rcc     :     86 us
  link        bench_rcc     :  54661 us

RCC -O1:
  preprocess  bench.c       :    684 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     27 us
  codegen     bench.c       :    140 us
  link        bench_o1      :     91 us
  link        bench_o1      :  57300 us

RCC -O2:
  preprocess  bench.c       :    642 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    124 us
  link        bench_o2      :    131 us
  link        bench_o2      :  49181 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 271248 us
  parse       sqlite3.c     : 153191 us
  typecheck   sqlite3.c     :  19520 us
  codegen     sqlite3.c     : 106095 us
  link        sqlite3.so    :  18078 us

RCC -O1:
  preprocess  sqlite3.c     : 238173 us
  parse       sqlite3.c     :  50617 us
  typecheck   sqlite3.c     :  13661 us
  opt         sqlite3.c     : 130715 us
  codegen     sqlite3.c     :  95266 us
  link        sqlite3.so    :  16002 us

RCC -O2:
  preprocess  sqlite3.c     : 222526 us
  parse       sqlite3.c     :  48554 us
  typecheck   sqlite3.c     :  12892 us
  opt         sqlite3.c     : 133462 us
  codegen     sqlite3.c     :  94314 us
  link        sqlite3.so    :  15763 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       936 ms |
| RCC -O1   |       852 ms |
| RCC -O2   |       722 ms |
| TCC       |       107 ms |
| GCC -O0   |      1048 ms |
| GCC -O2   |      9892 ms |
| Clang -O0 |      1076 ms |
| Clang -O2 |      9841 ms |
