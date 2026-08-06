# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          106 |          988 |       1094 |
| RCC -O1   |          181 |          792 |        973 |
| RCC -O2   |          142 |          876 |       1018 |
| TCC       |          146 |          810 |        956 |
| GCC -O0   |          168 |          682 |        850 |
| GCC -O2   |          199 |          340 |        539 |
| Clang -O0 |           87 |          524 |        611 |
| Clang -O2 |          135 |          310 |        445 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1219 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    115 us
  link        bench_rcc     :    242 us
  link        bench_rcc     :  66980 us

RCC -O1:
  preprocess  bench.c       :    650 us
  parse       bench.c       :    140 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    126 us
  link        bench_o1      :    265 us
  link        bench_o1      :  79584 us

RCC -O2:
  preprocess  bench.c       :    732 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    167 us
  link        bench_o2      :    260 us
  link        bench_o2      :  79425 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 389247 us
  parse       sqlite3.c     : 117731 us
  typecheck   sqlite3.c     :  30958 us
  codegen     sqlite3.c     : 123261 us
  link        sqlite3.so    :  19748 us

RCC -O1:
  preprocess  sqlite3.c     : 363504 us
  parse       sqlite3.c     :  49687 us
  typecheck   sqlite3.c     :  13065 us
  opt         sqlite3.c     :  21709 us
  codegen     sqlite3.c     : 119623 us
  link        sqlite3.so    :  16753 us

RCC -O2:
  preprocess  sqlite3.c     : 281771 us
  parse       sqlite3.c     :  50457 us
  typecheck   sqlite3.c     :  15126 us
  opt         sqlite3.c     : 193802 us
  codegen     sqlite3.c     : 130123 us
  link        sqlite3.so    :  21017 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       994 ms |
| RCC -O1   |       778 ms |
| RCC -O2   |       995 ms |
| TCC       |       128 ms |
| GCC -O0   |      1404 ms |
| GCC -O2   |     14544 ms |
| Clang -O0 |      1891 ms |
| Clang -O2 |     18920 ms |
