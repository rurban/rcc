# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          103 |          912 |       1015 |
| RCC -O1   |          115 |          878 |        993 |
| RCC -O2   |           95 |          721 |        816 |
| TCC       |           66 |          715 |        781 |
| GCC -O0   |          152 |          768 |        920 |
| GCC -O2   |          217 |          398 |        615 |
| Clang -O0 |          205 |          712 |        917 |
| Clang -O2 |          256 |          375 |        631 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1123 us
  parse       bench.c       :    150 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    150 us
  link        bench_rcc     :    400 us
  link        bench_rcc     :  65538 us

RCC -O1:
  preprocess  bench.c       :    653 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    748 us
  link        bench_o1      :  85810 us

RCC -O2:
  preprocess  bench.c       :    754 us
  parse       bench.c       :    130 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    128 us
  link        bench_o2      :    363 us
  link        bench_o2      :  68772 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 476418 us
  parse       sqlite3.c     : 229386 us
  typecheck   sqlite3.c     :  29914 us
  codegen     sqlite3.c     : 195250 us
  link        sqlite3.so    :  21421 us

RCC -O1:
  preprocess  sqlite3.c     : 385514 us
  parse       sqlite3.c     :  88031 us
  typecheck   sqlite3.c     :  37514 us
  opt         sqlite3.c     : 251980 us
  codegen     sqlite3.c     : 183656 us
  link        sqlite3.so    :  24870 us

RCC -O2:
  preprocess  sqlite3.c     : 343045 us
  parse       sqlite3.c     :  79649 us
  typecheck   sqlite3.c     :  16869 us
  opt         sqlite3.c     : 253518 us
  codegen     sqlite3.c     : 186611 us
  link        sqlite3.so    :  18767 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1793 ms |
| RCC -O1   |      2014 ms |
| RCC -O2   |      1339 ms |
| TCC       |       221 ms |
| GCC -O0   |      2154 ms |
| GCC -O2   |     18622 ms |
| Clang -O0 |      1978 ms |
| Clang -O2 |     17469 ms |
