# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           85 |          628 |        713 |
| RCC -O1   |           63 |          626 |        689 |
| RCC -O2   |           61 |          629 |        690 |
| TCC       |           81 |          568 |        649 |
| GCC -O0   |           99 |          467 |        566 |
| GCC -O2   |          110 |          286 |        396 |
| Clang -O0 |           74 |          470 |        544 |
| Clang -O2 |          137 |          300 |        437 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1038 us
  parse       bench.c       :    229 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    208 us
  link        bench_rcc     :    403 us
  link        bench_rcc     :  72536 us

RCC -O1:
  preprocess  bench.c       :    827 us
  parse       bench.c       :    153 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    126 us
  link        bench_o1      :    412 us
  link        bench_o1      :  63296 us

RCC -O2:
  preprocess  bench.c       :   1212 us
  parse       bench.c       :    296 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    176 us
  link        bench_o2      :    396 us
  link        bench_o2      :  63864 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 328342 us
  parse       sqlite3.c     :  98352 us
  typecheck   sqlite3.c     :  26460 us
  codegen     sqlite3.c     : 168353 us
  link        sqlite3.so    :  20436 us

RCC -O1:
  preprocess  sqlite3.c     : 334005 us
  parse       sqlite3.c     :  70045 us
  typecheck   sqlite3.c     :  13996 us
  opt         sqlite3.c     : 164944 us
  codegen     sqlite3.c     : 143231 us
  link        sqlite3.so    :  15719 us

RCC -O2:
  preprocess  sqlite3.c     : 269518 us
  parse       sqlite3.c     :  60378 us
  typecheck   sqlite3.c     :  16536 us
  opt         sqlite3.c     : 165210 us
  codegen     sqlite3.c     : 115385 us
  link        sqlite3.so    :  15848 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       850 ms |
| RCC -O1   |       906 ms |
| RCC -O2   |       923 ms |
| TCC       |       175 ms |
| GCC -O0   |      1618 ms |
| GCC -O2   |     11957 ms |
| Clang -O0 |      1253 ms |
| Clang -O2 |     12618 ms |
