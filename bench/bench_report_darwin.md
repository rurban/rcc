# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           89 |          671 |        760 |
| RCC -O1   |           70 |          631 |        701 |
| RCC -O2   |           98 |          749 |        847 |
| TCC       |           62 |          644 |        706 |
| GCC -O0   |           75 |          614 |        689 |
| GCC -O2   |          139 |          325 |        464 |
| Clang -O0 |          103 |          584 |        687 |
| Clang -O2 |          210 |          291 |        501 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    827 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    139 us
  link        bench_rcc     :    205 us
  link        bench_rcc     :  83323 us

RCC -O1:
  preprocess  bench.c       :    641 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    141 us
  link        bench_o1      :    328 us
  link        bench_o1      :  49821 us

RCC -O2:
  preprocess  bench.c       :    713 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    133 us
  link        bench_o2      :    410 us
  link        bench_o2      :  50134 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 280205 us
  parse       sqlite3.c     : 159177 us
  typecheck   sqlite3.c     :  23815 us
  codegen     sqlite3.c     : 119238 us
  link        sqlite3.so    :  14943 us

RCC -O1:
  preprocess  sqlite3.c     : 250123 us
  parse       sqlite3.c     :  58483 us
  typecheck   sqlite3.c     :  15845 us
  opt         sqlite3.c     : 199241 us
  codegen     sqlite3.c     : 110553 us
  link        sqlite3.so    :  15272 us

RCC -O2:
  preprocess  sqlite3.c     : 213003 us
  parse       sqlite3.c     :  51565 us
  typecheck   sqlite3.c     :  15845 us
  opt         sqlite3.c     : 206666 us
  codegen     sqlite3.c     : 174240 us
  link        sqlite3.so    :  16708 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       983 ms |
| RCC -O1   |       901 ms |
| RCC -O2   |       867 ms |
| TCC       |       121 ms |
| GCC -O0   |      1180 ms |
| GCC -O2   |     13485 ms |
| Clang -O0 |      1398 ms |
| Clang -O2 |     13885 ms |
