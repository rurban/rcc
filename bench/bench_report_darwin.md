# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          109 |          712 |        821 |
| RCC -O1   |           78 |          716 |        794 |
| RCC -O2   |           80 |          717 |        797 |
| TCC       |           78 |          629 |        707 |
| GCC -O0   |          152 |          535 |        687 |
| GCC -O2   |          132 |          318 |        450 |
| Clang -O0 |           92 |          538 |        630 |
| Clang -O2 |          127 |          327 |        454 |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          465 |         5691 |       6156 |
| RCC -O1   |          229 |         7064 |       7293 |
| RCC -O2   |          281 |         5692 |       5973 |
| TCC       |          169 |         4965 |       5134 |
| GCC -O0   |          610 |         3806 |       4416 |
| GCC -O2   |         1000 |         2045 |       3045 |
| Clang -O0 |          580 |         3837 |       4417 |
| Clang -O2 |          943 |         2253 |       3196 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    991 us
  parse       bench.c       :    228 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    175 us
  link        bench_rcc     :    341 us
  link        bench_rcc     :  68648 us

RCC -O1:
  preprocess  bench.c       :    760 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    160 us
  link        bench_o1      :    158 us
  link        bench_o1      :  63057 us

RCC -O2:
  preprocess  bench.c       :    773 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    187 us
  link        bench_o2      :    221 us
  link        bench_o2      :  69050 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 278034 us
  parse       sqlite3.c     :  79855 us
  typecheck   sqlite3.c     :  13782 us
  codegen     sqlite3.c     : 109727 us
  link        sqlite3.so    :  19772 us

RCC -O1:
  preprocess  sqlite3.c     : 285155 us
  parse       sqlite3.c     :  58378 us
  typecheck   sqlite3.c     :  18278 us
  opt         sqlite3.c     : 172223 us
  codegen     sqlite3.c     : 117860 us
  link        sqlite3.so    :  16722 us

RCC -O2:
  preprocess  sqlite3.c     : 255543 us
  parse       sqlite3.c     :  62177 us
  typecheck   sqlite3.c     :  18019 us
  opt         sqlite3.c     : 176656 us
  codegen     sqlite3.c     : 109741 us
  link        sqlite3.so    :  18175 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1332 ms |
| RCC -O1   |       933 ms |
| RCC -O2   |       923 ms |
| TCC       |       119 ms |
| GCC -O0   |      1185 ms |
| GCC -O2   |     12408 ms |
| Clang -O0 |      1142 ms |
| Clang -O2 |     11297 ms |
