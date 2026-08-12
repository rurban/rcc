# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           89 |          680 |        769 |
| RCC -O1   |           84 |          663 |        747 |
| RCC -O2   |           80 |          696 |        776 |
| TCC       |           68 |          627 |        695 |
| GCC -O0   |          125 |          472 |        597 |
| GCC -O2   |          126 |          313 |        439 |
| Clang -O0 |          102 |          480 |        582 |
| Clang -O2 |          163 |          337 |        500 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1348 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    174 us
  link        bench_rcc     :    219 us
  link        bench_rcc     :  87274 us

RCC -O1:
  preprocess  bench.c       :   1192 us
  parse       bench.c       :    165 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    196 us
  link        bench_o1      :    124 us
  link        bench_o1      :  71265 us

RCC -O2:
  preprocess  bench.c       :   1559 us
  parse       bench.c       :    217 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    174 us
  link        bench_o2      :    168 us
  link        bench_o2      :  55436 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 364053 us
  parse       sqlite3.c     :  77517 us
  typecheck   sqlite3.c     :  14361 us
  codegen     sqlite3.c     : 128675 us
  link        sqlite3.so    :  18468 us

RCC -O1:
  preprocess  sqlite3.c     : 267301 us
  parse       sqlite3.c     :  50872 us
  typecheck   sqlite3.c     :  12417 us
  opt         sqlite3.c     :  20061 us
  codegen     sqlite3.c     : 123808 us
  link        sqlite3.so    :  17893 us

RCC -O2:
  preprocess  sqlite3.c     : 289290 us
  parse       sqlite3.c     :  61816 us
  typecheck   sqlite3.c     :  17020 us
  opt         sqlite3.c     : 178802 us
  codegen     sqlite3.c     : 108778 us
  link        sqlite3.so    :  16671 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1427 ms |
| RCC -O1   |       934 ms |
| RCC -O2   |       974 ms |
| TCC       |       135 ms |
| GCC -O0   |      1591 ms |
| GCC -O2   |     14628 ms |
| Clang -O0 |      1684 ms |
| Clang -O2 |     12312 ms |
