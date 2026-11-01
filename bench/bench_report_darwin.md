# Darwin RCC Benchmark Results

_Generated: November 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           30 |          635 |        665 |    30% |
| RCC -O1   |           43 |          627 |        670 |     7% |
| RCC -O2   |           31 |          631 |        662 |     6% |
| TCC       |           26 |          545 |        571 |    84% |
| GCC -O0   |           83 |          451 |        534 |    46% |
| GCC -O2   |           99 |          273 |        372 |    10% |
| Clang -O0 |           62 |          455 |        517 |     9% |
| Clang -O2 |           96 |          279 |        375 |     5% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          159 |         4400 |       4559 |    33% |
| RCC -O1   |          139 |         5402 |       5541 |    30% |
| RCC -O2   |          188 |         4265 |       4453 |    44% |
| TCC       |          112 |         4310 |       4422 |    24% |
| GCC -O0   |          536 |         3347 |       3883 |     9% |
| GCC -O2   |          857 |         1993 |       2850 |    21% |
| Clang -O0 |          583 |         3502 |       4085 |    34% |
| Clang -O2 |          909 |         1937 |       2846 |    15% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    781 us
  parse       bench.c       :    173 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    182 us
  link        bench_rcc     :  32643 us

RCC -O1:
  preprocess  bench.c       :    846 us
  parse       bench.c       :    333 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     59 us
  codegen     bench.c       :    182 us
  link        bench_o1      :  27510 us

RCC -O2:
  preprocess  bench.c       :    599 us
  parse       bench.c       :    161 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     80 us
  codegen     bench.c       :    186 us
  link        bench_o2      :  26902 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 239820 us
  parse       sqlite3.c     :  59916 us
  typecheck   sqlite3.c     :  13131 us
  codegen     sqlite3.c     : 117492 us
  link        sqlite3.so    :  34484 us

RCC -O1:
  preprocess  sqlite3.c     : 223695 us
  parse       sqlite3.c     :  54463 us
  typecheck   sqlite3.c     :  14351 us
  opt         sqlite3.c     :  44920 us
  codegen     sqlite3.c     : 124828 us
  link        sqlite3.so    :  38974 us

RCC -O2:
  preprocess  sqlite3.c     : 228697 us
  parse       sqlite3.c     :  54890 us
  typecheck   sqlite3.c     :  18380 us
  opt         sqlite3.c     :  69989 us
  codegen     sqlite3.c     : 114848 us
  link        sqlite3.so    :  29631 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       505 ms |    31% |
| RCC -O1   |       474 ms |     1% |
| RCC -O2   |       483 ms |     6% |
| TCC       |        90 ms |     5% |
| GCC -O0   |      1008 ms |     5% |
| GCC -O2   |      9690 ms |     1% |
| Clang -O0 |       977 ms |     0% |
| Clang -O2 |      9504 ms |     0% |
