# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          180 |          796 |        976 |
| RCC -O1   |          111 |          836 |        947 |
| RCC -O2   |          102 |          783 |        885 |
| TCC       |           70 |          722 |        792 |
| GCC -O0   |          149 |          594 |        743 |
| GCC -O2   |          213 |          343 |        556 |
| Clang -O0 |          137 |          601 |        738 |
| Clang -O2 |          247 |          393 |        640 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1725 us
  parse       bench.c       :    418 us
  typecheck   bench.c       :      9 us
  codegen     bench.c       :    311 us
  link        bench_rcc     :    709 us
  link        bench_rcc     :  76362 us

RCC -O1:
  preprocess  bench.c       :    766 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    154 us
  link        bench_o1      :    201 us
  link        bench_o1      :  88060 us

RCC -O2:
  preprocess  bench.c       :    798 us
  parse       bench.c       :    156 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    168 us
  link        bench_o2      :    101 us
  link        bench_o2      :  58996 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 509585 us
  parse       sqlite3.c     :  91640 us
  typecheck   sqlite3.c     :  20020 us
  codegen     sqlite3.c     : 177935 us
  link        sqlite3.so    :  15928 us

RCC -O1:
  preprocess  sqlite3.c     : 464095 us
  parse       sqlite3.c     : 109726 us
  typecheck   sqlite3.c     :  20244 us
  opt         sqlite3.c     : 288907 us
  codegen     sqlite3.c     : 222363 us
  link        sqlite3.so    :  19112 us

RCC -O2:
  preprocess  sqlite3.c     : 439961 us
  parse       sqlite3.c     : 215758 us
  typecheck   sqlite3.c     :  28968 us
  opt         sqlite3.c     : 453813 us
  codegen     sqlite3.c     : 304723 us
  link        sqlite3.so    :  35735 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1947 ms |
| RCC -O1   |      1327 ms |
| RCC -O2   |      1351 ms |
| TCC       |       188 ms |
| GCC -O0   |      1488 ms |
| GCC -O2   |     16134 ms |
| Clang -O0 |      1648 ms |
| Clang -O2 |     17222 ms |
