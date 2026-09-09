# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           64 |          742 |        806 |    82% |
| RCC -O1   |           69 |          785 |        854 |    59% |
| RCC -O2   |           83 |          728 |        811 |   563% |
| TCC       |           45 |          686 |        731 |    51% |
| GCC -O0   |           88 |          579 |        667 |    45% |
| GCC -O2   |          120 |          356 |        476 |    27% |
| Clang -O0 |           81 |          594 |        675 |    22% |
| Clang -O2 |          140 |          349 |        489 |    62% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          193 |         7040 |       7233 |    56% |
| RCC -O1   |          215 |         6537 |       6752 |    42% |
| RCC -O2   |          204 |         5569 |       5773 |    90% |
| TCC       |          137 |         5041 |       5178 |   111% |
| GCC -O0   |          570 |         3685 |       4255 |    41% |
| GCC -O2   |         1019 |         2096 |       3115 |    11% |
| Clang -O0 |          583 |         3896 |       4479 |    21% |
| Clang -O2 |          903 |         2292 |       3195 |    22% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    799 us
  parse       bench.c       :    164 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    177 us
  link        bench_rcc     :    451 us
  link        bench_rcc     :  62526 us

RCC -O1:
  preprocess  bench.c       :    704 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    140 us
  link        bench_o1      :    169 us
  link        bench_o1      :  61069 us

RCC -O2:
  preprocess  bench.c       :    737 us
  parse       bench.c       :    137 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    116 us
  link        bench_o2      :     95 us
  link        bench_o2      :  61428 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 303813 us
  parse       sqlite3.c     :  91530 us
  typecheck   sqlite3.c     :  19523 us
  codegen     sqlite3.c     : 157887 us
  link        sqlite3.so    :  16824 us

RCC -O1:
  preprocess  sqlite3.c     : 259210 us
  parse       sqlite3.c     :  69323 us
  typecheck   sqlite3.c     :  13662 us
  opt         sqlite3.c     :  30127 us
  codegen     sqlite3.c     : 165481 us
  link        sqlite3.so    :  15606 us

RCC -O2:
  preprocess  sqlite3.c     : 340985 us
  parse       sqlite3.c     :  90184 us
  typecheck   sqlite3.c     :  23737 us
  opt         sqlite3.c     :  36187 us
  codegen     sqlite3.c     : 179513 us
  link        sqlite3.so    :  22865 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       611 ms |    23% |
| RCC -O1   |       544 ms |     7% |
| RCC -O2   |       459 ms |     3% |
| TCC       |        95 ms |    34% |
| GCC -O0   |      1145 ms |     4% |
| GCC -O2   |     12066 ms |     4% |
| Clang -O0 |      1133 ms |    16% |
| Clang -O2 |     10705 ms |     5% |
