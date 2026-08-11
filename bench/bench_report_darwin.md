# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          705 |        802 |
| RCC -O1   |           64 |          707 |        771 |
| RCC -O2   |           88 |          748 |        836 |
| TCC       |           90 |          610 |        700 |
| GCC -O0   |           89 |          517 |        606 |
| GCC -O2   |          165 |          317 |        482 |
| Clang -O0 |           70 |          516 |        586 |
| Clang -O2 |          122 |          327 |        449 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    788 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    149 us
  link        bench_rcc     :    142 us
  link        bench_rcc     :  62247 us

RCC -O1:
  preprocess  bench.c       :    599 us
  parse       bench.c       :    265 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     59 us
  codegen     bench.c       :    138 us
  link        bench_o1      :    148 us
  link        bench_o1      :  59535 us

RCC -O2:
  preprocess  bench.c       :    926 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    118 us
  link        bench_o2      :     81 us
  link        bench_o2      :  56100 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 327323 us
  parse       sqlite3.c     : 201779 us
  typecheck   sqlite3.c     :  23409 us
  codegen     sqlite3.c     : 124574 us
  link        sqlite3.so    :  22661 us

RCC -O1:
  preprocess  sqlite3.c     : 336123 us
  parse       sqlite3.c     :  59580 us
  typecheck   sqlite3.c     :  19841 us
  opt         sqlite3.c     :  21790 us
  codegen     sqlite3.c     : 149860 us
  link        sqlite3.so    :  21113 us

RCC -O2:
  preprocess  sqlite3.c     : 310501 us
  parse       sqlite3.c     :  56272 us
  typecheck   sqlite3.c     :  14114 us
  opt         sqlite3.c     : 162639 us
  codegen     sqlite3.c     : 138769 us
  link        sqlite3.so    :  26195 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1090 ms |
| RCC -O1   |      1118 ms |
| RCC -O2   |      1272 ms |
| TCC       |       188 ms |
| GCC -O0   |      1718 ms |
| GCC -O2   |     13120 ms |
| Clang -O0 |      1197 ms |
| Clang -O2 |     12074 ms |
