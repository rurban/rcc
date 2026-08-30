# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          637 |        690 |
| RCC -O1   |           53 |          639 |        692 |
| RCC -O2   |           60 |          642 |        702 |
| TCC       |           45 |          555 |        600 |
| GCC -O0   |           66 |          468 |        534 |
| GCC -O2   |          103 |          289 |        392 |
| Clang -O0 |           56 |          469 |        525 |
| Clang -O2 |           93 |          284 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    869 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    128 us
  link        bench_rcc     :    135 us
  link        bench_rcc     :  60084 us

RCC -O1:
  preprocess  bench.c       :    611 us
  parse       bench.c       :    148 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    144 us
  link        bench_o1      :  53831 us

RCC -O2:
  preprocess  bench.c       :    645 us
  parse       bench.c       :    144 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    158 us
  link        bench_o2      :    221 us
  link        bench_o2      :  53954 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 219294 us
  parse       sqlite3.c     :  51464 us
  typecheck   sqlite3.c     :  11506 us
  codegen     sqlite3.c     : 118998 us
  link        sqlite3.so    :  15788 us

RCC -O1:
  preprocess  sqlite3.c     : 231736 us
  parse       sqlite3.c     :  58302 us
  typecheck   sqlite3.c     :  14753 us
  opt         sqlite3.c     : 141968 us
  codegen     sqlite3.c     :  98681 us
  link        sqlite3.so    :  14828 us

RCC -O2:
  preprocess  sqlite3.c     : 193076 us
  parse       sqlite3.c     :  49127 us
  typecheck   sqlite3.c     :  10538 us
  opt         sqlite3.c     : 143021 us
  codegen     sqlite3.c     :  97949 us
  link        sqlite3.so    :  15480 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       716 ms |
| RCC -O1   |       712 ms |
| RCC -O2   |       715 ms |
| TCC       |        93 ms |
| GCC -O0   |      1074 ms |
| GCC -O2   |     10237 ms |
| Clang -O0 |      1047 ms |
| Clang -O2 |      9794 ms |
