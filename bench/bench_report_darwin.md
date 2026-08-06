# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          173 |          811 |        984 |
| RCC -O1   |           89 |          774 |        863 |
| RCC -O2   |           91 |          769 |        860 |
| TCC       |           89 |          684 |        773 |
| GCC -O0   |          134 |          600 |        734 |
| GCC -O2   |          205 |          349 |        554 |
| Clang -O0 |          133 |          606 |        739 |
| Clang -O2 |          191 |          330 |        521 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1123 us
  parse       bench.c       :    226 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    180 us
  link        bench_rcc     :    370 us
  link        bench_rcc     :  86669 us

RCC -O1:
  preprocess  bench.c       :    694 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    111 us
  link        bench_o1      :    136 us
  link        bench_o1      :  95718 us

RCC -O2:
  preprocess  bench.c       :   1335 us
  parse       bench.c       :    244 us
  typecheck   bench.c       :     10 us
  opt         bench.c       :     43 us
  codegen     bench.c       :    308 us
  link        bench_o2      :    424 us
  link        bench_o2      :  91273 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 558544 us
  parse       sqlite3.c     : 266131 us
  typecheck   sqlite3.c     :  23566 us
  codegen     sqlite3.c     : 199433 us
  link        sqlite3.so    :  19497 us

RCC -O1:
  preprocess  sqlite3.c     : 448326 us
  parse       sqlite3.c     :  71796 us
  typecheck   sqlite3.c     :  18835 us
  opt         sqlite3.c     :  23298 us
  codegen     sqlite3.c     : 370959 us
  link        sqlite3.so    : 285663 us

RCC -O2:
  preprocess  sqlite3.c     : 444672 us
  parse       sqlite3.c     : 103631 us
  typecheck   sqlite3.c     :  24755 us
  opt         sqlite3.c     : 361646 us
  codegen     sqlite3.c     : 233844 us
  link        sqlite3.so    :  20819 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       952 ms |
| RCC -O1   |       816 ms |
| RCC -O2   |      1084 ms |
| TCC       |       124 ms |
| GCC -O0   |      1397 ms |
| GCC -O2   |     15885 ms |
| Clang -O0 |      2263 ms |
| Clang -O2 |     15996 ms |
