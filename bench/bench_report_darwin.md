# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           70 |          713 |        783 |    18% |
| RCC -O1   |           58 |          688 |        746 |    21% |
| RCC -O2   |           88 |          783 |        871 |    18% |
| TCC       |           34 |          655 |        689 |   144% |
| GCC -O0   |          144 |          645 |        789 |   115% |
| GCC -O2   |          173 |          380 |        553 |    41% |
| Clang -O0 |           83 |          564 |        647 |   100% |
| Clang -O2 |          137 |          352 |        489 |    24% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          181 |         6917 |       7098 |    53% |
| RCC -O1   |          234 |         7104 |       7338 |    17% |
| RCC -O2   |          167 |         6369 |       6536 |    88% |
| TCC       |          133 |         5649 |       5782 |   106% |
| GCC -O0   |          896 |         4460 |       5356 |    56% |
| GCC -O2   |          968 |         2190 |       3158 |    20% |
| Clang -O0 |          579 |         3654 |       4233 |    40% |
| Clang -O2 |          941 |         2228 |       3169 |    18% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1462 us
  parse       bench.c       :    178 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    144 us
  link        bench_rcc     :    118 us
  link        bench_rcc     :  64441 us

RCC -O1:
  preprocess  bench.c       :    714 us
  parse       bench.c       :    185 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    121 us
  link        bench_o1      :    164 us
  link        bench_o1      :  68447 us

RCC -O2:
  preprocess  bench.c       :    718 us
  parse       bench.c       :    170 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    135 us
  link        bench_o2      :    187 us
  link        bench_o2      :  63957 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 292325 us
  parse       sqlite3.c     : 131463 us
  typecheck   sqlite3.c     :  25907 us
  codegen     sqlite3.c     : 133961 us
  link        sqlite3.so    :  18890 us

RCC -O1:
  preprocess  sqlite3.c     : 320230 us
  parse       sqlite3.c     :  75249 us
  typecheck   sqlite3.c     :  13850 us
  opt         sqlite3.c     :  28953 us
  codegen     sqlite3.c     : 120722 us
  link        sqlite3.so    :  28388 us

RCC -O2:
  preprocess  sqlite3.c     : 335329 us
  parse       sqlite3.c     :  77691 us
  typecheck   sqlite3.c     :  31170 us
  opt         sqlite3.c     :  41140 us
  codegen     sqlite3.c     : 153094 us
  link        sqlite3.so    :  17456 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       616 ms |    27% |
| RCC -O1   |       538 ms |     0% |
| RCC -O2   |       467 ms |    17% |
| TCC       |        98 ms |    17% |
| GCC -O0   |       984 ms |     9% |
| GCC -O2   |      9450 ms |    24% |
| Clang -O0 |      1565 ms |    29% |
| Clang -O2 |     12595 ms |    28% |
