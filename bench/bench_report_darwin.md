# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           59 |          682 |        741 |    27% |
| RCC -O1   |           72 |          670 |        742 |     9% |
| RCC -O2   |           64 |          685 |        749 |     7% |
| TCC       |           30 |          588 |        618 |   133% |
| GCC -O0   |           63 |          458 |        521 |    12% |
| GCC -O2   |          108 |          278 |        386 |    22% |
| Clang -O0 |           56 |          460 |        516 |     8% |
| Clang -O2 |           87 |          277 |        364 |     3% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          125 |         4702 |       4827 |     8% |
| RCC -O1   |          153 |         4813 |       4966 |    77% |
| RCC -O2   |          178 |         5570 |       5748 |    17% |
| TCC       |          143 |         5150 |       5293 |    20% |
| GCC -O0   |          588 |         3699 |       4287 |    16% |
| GCC -O2   |          983 |         2324 |       3307 |    16% |
| Clang -O0 |          564 |         3664 |       4228 |    27% |
| Clang -O2 |          966 |         2292 |       3258 |    11% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1059 us
  parse       bench.c       :    250 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    198 us
  link        bench_rcc     :    313 us
  link        bench_rcc     :  60508 us

RCC -O1:
  preprocess  bench.c       :    717 us
  parse       bench.c       :    198 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     33 us
  codegen     bench.c       :    160 us
  link        bench_o1      :     89 us
  link        bench_o1      :  54266 us

RCC -O2:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    125 us
  link        bench_o2      :    288 us
  link        bench_o2      :  57476 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 254272 us
  parse       sqlite3.c     :  82350 us
  typecheck   sqlite3.c     :  23375 us
  codegen     sqlite3.c     : 140998 us
  link        sqlite3.so    :  24974 us

RCC -O1:
  preprocess  sqlite3.c     : 260539 us
  parse       sqlite3.c     :  61884 us
  typecheck   sqlite3.c     :  16660 us
  opt         sqlite3.c     :  25162 us
  codegen     sqlite3.c     : 118565 us
  link        sqlite3.so    :  21417 us

RCC -O2:
  preprocess  sqlite3.c     : 244044 us
  parse       sqlite3.c     :  63076 us
  typecheck   sqlite3.c     :  15164 us
  opt         sqlite3.c     :  29208 us
  codegen     sqlite3.c     : 158275 us
  link        sqlite3.so    :  17214 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       555 ms |    58% |
| RCC -O1   |       489 ms |     1% |
| RCC -O2   |       584 ms |     2% |
| TCC       |       121 ms |    20% |
| GCC -O0   |      1147 ms |    10% |
| GCC -O2   |     12230 ms |    10% |
| Clang -O0 |      1273 ms |     7% |
| Clang -O2 |     11993 ms |     6% |
