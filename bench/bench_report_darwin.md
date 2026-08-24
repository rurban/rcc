# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          200 |          680 |        880 |
| RCC -O1   |           70 |          762 |        832 |
| RCC -O2   |          169 |          706 |        875 |
| TCC       |           55 |          712 |        767 |
| GCC -O0   |           87 |          617 |        704 |
| GCC -O2   |          156 |          353 |        509 |
| Clang -O0 |          102 |          666 |        768 |
| Clang -O2 |          198 |          395 |        593 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1323 us
  parse       bench.c       :    217 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    179 us
  link        bench_rcc     :    244 us
  link        bench_rcc     :  64861 us

RCC -O1:
  preprocess  bench.c       :   1150 us
  parse       bench.c       :    176 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    193 us
  link        bench_o1      :    130 us
  link        bench_o1      :  78453 us

RCC -O2:
  preprocess  bench.c       :    837 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    161 us
  link        bench_o2      :     82 us
  link        bench_o2      :  50471 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 304257 us
  parse       sqlite3.c     :  91680 us
  typecheck   sqlite3.c     :  32527 us
  codegen     sqlite3.c     : 144696 us
  link        sqlite3.so    :  18477 us

RCC -O1:
  preprocess  sqlite3.c     : 325154 us
  parse       sqlite3.c     : 113286 us
  typecheck   sqlite3.c     :  33638 us
  opt         sqlite3.c     : 379486 us
  codegen     sqlite3.c     : 278652 us
  link        sqlite3.so    :  30591 us

RCC -O2:
  preprocess  sqlite3.c     : 475157 us
  parse       sqlite3.c     : 122183 us
  typecheck   sqlite3.c     :  30513 us
  opt         sqlite3.c     : 369379 us
  codegen     sqlite3.c     : 161774 us
  link        sqlite3.so    :  75791 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1553 ms |
| RCC -O1   |      1137 ms |
| RCC -O2   |      1355 ms |
| TCC       |       200 ms |
| GCC -O0   |      2272 ms |
| GCC -O2   |     19138 ms |
| Clang -O0 |      1728 ms |
| Clang -O2 |     14235 ms |
