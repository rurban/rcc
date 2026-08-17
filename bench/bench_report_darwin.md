# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           96 |          851 |        947 |
| RCC -O1   |          107 |          804 |        911 |
| RCC -O2   |          160 |          841 |       1001 |
| TCC       |           84 |          749 |        833 |
| GCC -O0   |          153 |          589 |        742 |
| GCC -O2   |          188 |          362 |        550 |
| Clang -O0 |          100 |          535 |        635 |
| Clang -O2 |          164 |          325 |        489 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1251 us
  parse       bench.c       :    205 us
  typecheck   bench.c       :      7 us
  codegen     bench.c       :    227 us
  link        bench_rcc     :    242 us
  link        bench_rcc     :  86778 us

RCC -O1:
  preprocess  bench.c       :    922 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    124 us
  link        bench_o1      :    423 us
  link        bench_o1      :  91673 us

RCC -O2:
  preprocess  bench.c       :   1552 us
  parse       bench.c       :    354 us
  typecheck   bench.c       :     14 us
  opt         bench.c       :     32 us
  codegen     bench.c       :    165 us
  link        bench_o2      :    901 us
  link        bench_o2      :  76913 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 443612 us
  parse       sqlite3.c     : 127451 us
  typecheck   sqlite3.c     :  20525 us
  codegen     sqlite3.c     : 178267 us
  link        sqlite3.so    :  24822 us

RCC -O1:
  preprocess  sqlite3.c     : 380871 us
  parse       sqlite3.c     : 105535 us
  typecheck   sqlite3.c     :  38679 us
  opt         sqlite3.c     : 257831 us
  codegen     sqlite3.c     : 215461 us
  link        sqlite3.so    :  26028 us

RCC -O2:
  preprocess  sqlite3.c     : 393827 us
  parse       sqlite3.c     :  68192 us
  typecheck   sqlite3.c     :  22953 us
  opt         sqlite3.c     : 256482 us
  codegen     sqlite3.c     : 233920 us
  link        sqlite3.so    :  24092 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       998 ms |
| RCC -O1   |       948 ms |
| RCC -O2   |       843 ms |
| TCC       |       125 ms |
| GCC -O0   |      1475 ms |
| GCC -O2   |     12245 ms |
| Clang -O0 |      1301 ms |
| Clang -O2 |     14221 ms |
