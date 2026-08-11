# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           64 |          814 |        878 |
| RCC -O1   |           94 |          791 |        885 |
| RCC -O2   |          106 |          912 |       1018 |
| TCC       |          256 |          670 |        926 |
| GCC -O0   |          144 |          540 |        684 |
| GCC -O2   |          135 |          302 |        437 |
| Clang -O0 |           76 |          511 |        587 |
| Clang -O2 |          128 |          287 |        415 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    877 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    140 us
  link        bench_rcc     :    194 us
  link        bench_rcc     :  74410 us

RCC -O1:
  preprocess  bench.c       :   1257 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    127 us
  link        bench_o1      :    135 us
  link        bench_o1      :  56351 us

RCC -O2:
  preprocess  bench.c       :   1706 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    125 us
  link        bench_o2      :    163 us
  link        bench_o2      :  56873 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 320596 us
  parse       sqlite3.c     :  86127 us
  typecheck   sqlite3.c     :  18857 us
  codegen     sqlite3.c     : 164655 us
  link        sqlite3.so    :  18751 us

RCC -O1:
  preprocess  sqlite3.c     : 367417 us
  parse       sqlite3.c     :  80900 us
  typecheck   sqlite3.c     :  18855 us
  opt         sqlite3.c     :  22442 us
  codegen     sqlite3.c     : 129251 us
  link        sqlite3.so    :  20870 us

RCC -O2:
  preprocess  sqlite3.c     : 327074 us
  parse       sqlite3.c     :  65162 us
  typecheck   sqlite3.c     :  23061 us
  opt         sqlite3.c     : 197458 us
  codegen     sqlite3.c     : 153629 us
  link        sqlite3.so    :  16512 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1191 ms |
| RCC -O1   |       823 ms |
| RCC -O2   |       820 ms |
| TCC       |       134 ms |
| GCC -O0   |      1151 ms |
| GCC -O2   |     18357 ms |
| Clang -O0 |      2188 ms |
| Clang -O2 |     10383 ms |
