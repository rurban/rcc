# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           56 |          696 |        752 |    66% |
| RCC -O1   |           60 |          671 |        731 |     5% |
| RCC -O2   |           64 |          685 |        749 |     3% |
| TCC       |           29 |          592 |        621 |    72% |
| GCC -O0   |           71 |          479 |        550 |     6% |
| GCC -O2   |           96 |          290 |        386 |    19% |
| Clang -O0 |           59 |          466 |        525 |     1% |
| Clang -O2 |           90 |          279 |        369 |     8% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          128 |         4894 |       5022 |    10% |
| RCC -O1   |          140 |         4790 |       4930 |    18% |
| RCC -O2   |          129 |         4280 |       4409 |    46% |
| TCC       |          104 |         4149 |       4253 |    45% |
| GCC -O0   |          500 |         3369 |       3869 |     3% |
| GCC -O2   |          867 |         1926 |       2793 |     5% |
| Clang -O0 |          492 |         3404 |       3896 |     2% |
| Clang -O2 |          857 |         1934 |       2791 |     1% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    641 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    131 us
  link        bench_rcc     :    105 us
  link        bench_rcc     :  53133 us

RCC -O1:
  preprocess  bench.c       :    562 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     49 us
  codegen     bench.c       :    126 us
  link        bench_o1      :     97 us
  link        bench_o1      :  52035 us

RCC -O2:
  preprocess  bench.c       :    551 us
  parse       bench.c       :    133 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     52 us
  codegen     bench.c       :    135 us
  link        bench_o2      :    141 us
  link        bench_o2      :  50818 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 207120 us
  parse       sqlite3.c     :  61612 us
  typecheck   sqlite3.c     :  20578 us
  codegen     sqlite3.c     : 116632 us
  link        sqlite3.so    :  21862 us

RCC -O1:
  preprocess  sqlite3.c     : 226210 us
  parse       sqlite3.c     :  56343 us
  typecheck   sqlite3.c     :  12818 us
  opt         sqlite3.c     :  47695 us
  codegen     sqlite3.c     : 130181 us
  link        sqlite3.so    :  14843 us

RCC -O2:
  preprocess  sqlite3.c     : 220820 us
  parse       sqlite3.c     :  58027 us
  typecheck   sqlite3.c     :  13016 us
  opt         sqlite3.c     :  50286 us
  codegen     sqlite3.c     : 142581 us
  link        sqlite3.so    :  16715 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       427 ms |    20% |
| RCC -O1   |       451 ms |     3% |
| RCC -O2   |       455 ms |     0% |
| TCC       |        93 ms |     7% |
| GCC -O0   |      1024 ms |     2% |
| GCC -O2   |      9378 ms |     3% |
| Clang -O0 |       997 ms |     1% |
| Clang -O2 |      9505 ms |     1% |
