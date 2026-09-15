# Darwin RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           53 |          737 |        790 |    60% |
| RCC -O1   |           57 |          701 |        758 |    43% |
| RCC -O2   |           64 |          659 |        723 |    32% |
| TCC       |           25 |          583 |        608 |   108% |
| GCC -O0   |           72 |          493 |        565 |    59% |
| GCC -O2   |           92 |          293 |        385 |    13% |
| Clang -O0 |           61 |          458 |        519 |    22% |
| Clang -O2 |           93 |          283 |        376 |    20% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          123 |         4502 |       4625 |    79% |
| RCC -O1   |          242 |         5879 |       6121 |    34% |
| RCC -O2   |          148 |         4993 |       5141 |   110% |
| TCC       |          129 |         4719 |       4848 |    49% |
| GCC -O0   |          498 |         3712 |       4210 |    23% |
| GCC -O2   |          912 |         2200 |       3112 |    13% |
| Clang -O0 |          558 |         3735 |       4293 |     5% |
| Clang -O2 |         1034 |         2291 |       3325 |     9% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    781 us
  parse       bench.c       :    202 us
  typecheck   bench.c       :      3 us
  codegen     bench.c       :    192 us
  link        bench_rcc     :    113 us
  link        bench_rcc     :  52309 us

RCC -O1:
  preprocess  bench.c       :    565 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    126 us
  link        bench_o1      :    352 us
  link        bench_o1      :  49340 us

RCC -O2:
  preprocess  bench.c       :    618 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    239 us
  link        bench_o2      :    199 us
  link        bench_o2      :  53552 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 259813 us
  parse       sqlite3.c     : 227150 us
  typecheck   sqlite3.c     :  19715 us
  codegen     sqlite3.c     : 304311 us
  link        sqlite3.so    :  21046 us

RCC -O1:
  preprocess  sqlite3.c     : 312254 us
  parse       sqlite3.c     :  74358 us
  typecheck   sqlite3.c     :  15783 us
  opt         sqlite3.c     :  29134 us
  codegen     sqlite3.c     : 145085 us
  link        sqlite3.so    :  21208 us

RCC -O2:
  preprocess  sqlite3.c     : 238561 us
  parse       sqlite3.c     :  74678 us
  typecheck   sqlite3.c     :  12557 us
  opt         sqlite3.c     :  36102 us
  codegen     sqlite3.c     : 118619 us
  link        sqlite3.so    :  15794 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       559 ms |    49% |
| RCC -O1   |       512 ms |     3% |
| RCC -O2   |       492 ms |    30% |
| TCC       |       111 ms |    26% |
| GCC -O0   |      1274 ms |     8% |
| GCC -O2   |     11084 ms |     3% |
| Clang -O0 |      1138 ms |     9% |
| Clang -O2 |     12186 ms |    13% |
