# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          141 |         1149 |       1290 |    38% |
| RCC -O1   |          159 |          784 |        943 |    61% |
| RCC -O2   |           95 |          741 |        836 |    31% |
| TCC       |           45 |          676 |        721 |    48% |
| GCC -O0   |          153 |          629 |        782 |    59% |
| GCC -O2   |          148 |          305 |        453 |    58% |
| Clang -O0 |           90 |          502 |        592 |    28% |
| Clang -O2 |          109 |          308 |        417 |    36% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          145 |         6115 |       6260 |    95% |
| RCC -O1   |          130 |         6244 |       6374 |   140% |
| RCC -O2   |          150 |         5124 |       5274 |   102% |
| TCC       |          147 |         5611 |       5758 |    69% |
| GCC -O0   |          608 |         3862 |       4470 |    14% |
| GCC -O2   |         1000 |         2713 |       3713 |    14% |
| Clang -O0 |          599 |         4257 |       4856 |    19% |
| Clang -O2 |         1027 |         2415 |       3442 |    18% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   2676 us
  parse       bench.c       :    423 us
  typecheck   bench.c       :      8 us
  codegen     bench.c       :    376 us
  link        bench_rcc     :    416 us
  link        bench_rcc     :  96550 us

RCC -O1:
  preprocess  bench.c       :    785 us
  parse       bench.c       :    167 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    149 us
  link        bench_o1      :    913 us
  link        bench_o1      :  92849 us

RCC -O2:
  preprocess  bench.c       :   1613 us
  parse       bench.c       :    390 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     44 us
  codegen     bench.c       :    173 us
  link        bench_o2      :    263 us
  link        bench_o2      :  87467 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 428095 us
  parse       sqlite3.c     : 348885 us
  typecheck   sqlite3.c     :  28897 us
  codegen     sqlite3.c     : 336954 us
  link        sqlite3.so    :  26326 us

RCC -O1:
  preprocess  sqlite3.c     : 655031 us
  parse       sqlite3.c     : 151713 us
  typecheck   sqlite3.c     :  27976 us
  opt         sqlite3.c     :  79836 us
  codegen     sqlite3.c     : 236301 us
  link        sqlite3.so    :  33583 us

RCC -O2:
  preprocess  sqlite3.c     : 377769 us
  parse       sqlite3.c     : 122659 us
  typecheck   sqlite3.c     :  29600 us
  opt         sqlite3.c     :  59120 us
  codegen     sqlite3.c     : 257705 us
  link        sqlite3.so    :  24949 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       569 ms |    79% |
| RCC -O1   |       482 ms |     5% |
| RCC -O2   |       494 ms |     0% |
| TCC       |       107 ms |    25% |
| GCC -O0   |      1329 ms |     4% |
| GCC -O2   |     12866 ms |     1% |
| Clang -O0 |      1210 ms |     3% |
| Clang -O2 |     11812 ms |     7% |
