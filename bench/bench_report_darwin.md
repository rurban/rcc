# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          193 |          799 |        992 |
| RCC -O1   |          103 |          824 |        927 |
| RCC -O2   |          114 |          940 |       1054 |
| TCC       |           62 |          747 |        809 |
| GCC -O0   |          170 |          598 |        768 |
| GCC -O2   |          184 |          366 |        550 |
| Clang -O0 |          115 |          667 |        782 |
| Clang -O2 |          195 |          360 |        555 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1240 us
  parse       bench.c       :    192 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    157 us
  link        bench_rcc     :    186 us
  link        bench_rcc     :  83134 us

RCC -O1:
  preprocess  bench.c       :    660 us
  parse       bench.c       :    149 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    129 us
  link        bench_o1      :    620 us
  link        bench_o1      :  86026 us

RCC -O2:
  preprocess  bench.c       :    729 us
  parse       bench.c       :    188 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     29 us
  codegen     bench.c       :    140 us
  link        bench_o2      :    567 us
  link        bench_o2      :  95669 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 468992 us
  parse       sqlite3.c     : 237221 us
  typecheck   sqlite3.c     :  34849 us
  codegen     sqlite3.c     : 268237 us
  link        sqlite3.so    :  40727 us

RCC -O1:
  preprocess  sqlite3.c     : 418177 us
  parse       sqlite3.c     :  99206 us
  typecheck   sqlite3.c     :  21300 us
  opt         sqlite3.c     : 287228 us
  codegen     sqlite3.c     : 249751 us
  link        sqlite3.so    :  34001 us

RCC -O2:
  preprocess  sqlite3.c     : 420091 us
  parse       sqlite3.c     :  83365 us
  typecheck   sqlite3.c     :  31866 us
  opt         sqlite3.c     : 409944 us
  codegen     sqlite3.c     : 231954 us
  link        sqlite3.so    :  22991 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1387 ms |
| RCC -O1   |      1210 ms |
| RCC -O2   |      1280 ms |
| TCC       |       274 ms |
| GCC -O0   |      1660 ms |
| GCC -O2   |     16409 ms |
| Clang -O0 |      1677 ms |
| Clang -O2 |     16195 ms |
