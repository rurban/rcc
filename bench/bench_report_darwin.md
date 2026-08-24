# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           75 |          708 |        783 |
| RCC -O1   |           91 |          805 |        896 |
| RCC -O2   |          104 |          695 |        799 |
| TCC       |           77 |          584 |        661 |
| GCC -O0   |           91 |          562 |        653 |
| GCC -O2   |          150 |          323 |        473 |
| Clang -O0 |           75 |          574 |        649 |
| Clang -O2 |          172 |          356 |        528 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1046 us
  parse       bench.c       :    332 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    196 us
  link        bench_rcc     :    181 us
  link        bench_rcc     :  70433 us

RCC -O1:
  preprocess  bench.c       :    823 us
  parse       bench.c       :    240 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     31 us
  codegen     bench.c       :    171 us
  link        bench_o1      :    230 us
  link        bench_o1      :  68153 us

RCC -O2:
  preprocess  bench.c       :    817 us
  parse       bench.c       :    168 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     28 us
  codegen     bench.c       :    164 us
  link        bench_o2      :    195 us
  link        bench_o2      :  74385 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 338858 us
  parse       sqlite3.c     :  85528 us
  typecheck   sqlite3.c     :  17252 us
  codegen     sqlite3.c     : 117745 us
  link        sqlite3.so    :  19232 us

RCC -O1:
  preprocess  sqlite3.c     : 261878 us
  parse       sqlite3.c     :  60570 us
  typecheck   sqlite3.c     :  17060 us
  opt         sqlite3.c     : 158901 us
  codegen     sqlite3.c     : 120023 us
  link        sqlite3.so    :  17105 us

RCC -O2:
  preprocess  sqlite3.c     : 243320 us
  parse       sqlite3.c     :  56429 us
  typecheck   sqlite3.c     :  14670 us
  opt         sqlite3.c     : 165656 us
  codegen     sqlite3.c     : 120919 us
  link        sqlite3.so    :  19272 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1019 ms |
| RCC -O1   |      1147 ms |
| RCC -O2   |      1063 ms |
| TCC       |       155 ms |
| GCC -O0   |      1774 ms |
| GCC -O2   |     15163 ms |
| Clang -O0 |      1591 ms |
| Clang -O2 |     10961 ms |
