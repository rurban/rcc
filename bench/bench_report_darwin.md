# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          203 |          992 |       1195 |
| RCC -O1   |          136 |          857 |        993 |
| RCC -O2   |          154 |          916 |       1070 |
| TCC       |           81 |          861 |        942 |
| GCC -O0   |          169 |          660 |        829 |
| GCC -O2   |          252 |          384 |        636 |
| Clang -O0 |           81 |          629 |        710 |
| Clang -O2 |          153 |          307 |        460 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   4703 us
  parse       bench.c       :    246 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    393 us
  link        bench_rcc     :    211 us
  link        bench_rcc     :  99354 us

RCC -O1:
  preprocess  bench.c       :   1989 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    165 us
  link        bench_o1      :    902 us
  link        bench_o1      :  90538 us

RCC -O2:
  preprocess  bench.c       :    809 us
  parse       bench.c       :    194 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    163 us
  link        bench_o2      :    629 us
  link        bench_o2      : 102012 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 471806 us
  parse       sqlite3.c     :  80480 us
  typecheck   sqlite3.c     :  23656 us
  codegen     sqlite3.c     : 168452 us
  link        sqlite3.so    :  23375 us

RCC -O1:
  preprocess  sqlite3.c     : 279575 us
  parse       sqlite3.c     :  71331 us
  typecheck   sqlite3.c     :  26153 us
  opt         sqlite3.c     : 227723 us
  codegen     sqlite3.c     : 189187 us
  link        sqlite3.so    :  30040 us

RCC -O2:
  preprocess  sqlite3.c     : 291072 us
  parse       sqlite3.c     :  84912 us
  typecheck   sqlite3.c     :  30910 us
  opt         sqlite3.c     : 245173 us
  codegen     sqlite3.c     : 214949 us
  link        sqlite3.so    :  26661 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1070 ms |
| RCC -O1   |      1046 ms |
| RCC -O2   |      1157 ms |
| TCC       |       128 ms |
| GCC -O0   |      1429 ms |
| GCC -O2   |     12720 ms |
| Clang -O0 |      1460 ms |
| Clang -O2 |     12287 ms |
