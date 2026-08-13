# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           79 |          709 |        788 |
| RCC -O1   |          129 |          788 |        917 |
| RCC -O2   |           84 |          773 |        857 |
| TCC       |           84 |          737 |        821 |
| GCC -O0   |          153 |          652 |        805 |
| GCC -O2   |          296 |          400 |        696 |
| Clang -O0 |          183 |          736 |        919 |
| Clang -O2 |          367 |          467 |        834 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    928 us
  parse       bench.c       :    159 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    177 us
  link        bench_rcc     :    215 us
  link        bench_rcc     :  75653 us

RCC -O1:
  preprocess  bench.c       :   1585 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    133 us
  link        bench_o1      :    174 us
  link        bench_o1      :  68374 us

RCC -O2:
  preprocess  bench.c       :    954 us
  parse       bench.c       :    155 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    151 us
  link        bench_o2      :    245 us
  link        bench_o2      :  74322 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 452229 us
  parse       sqlite3.c     : 216569 us
  typecheck   sqlite3.c     :  27410 us
  codegen     sqlite3.c     : 280145 us
  link        sqlite3.so    :  28264 us

RCC -O1:
  preprocess  sqlite3.c     : 330753 us
  parse       sqlite3.c     :  52078 us
  typecheck   sqlite3.c     :  12935 us
  opt         sqlite3.c     : 214754 us
  codegen     sqlite3.c     : 113310 us
  link        sqlite3.so    :  16644 us

RCC -O2:
  preprocess  sqlite3.c     : 225344 us
  parse       sqlite3.c     :  49579 us
  typecheck   sqlite3.c     :  14554 us
  opt         sqlite3.c     : 149243 us
  codegen     sqlite3.c     : 126003 us
  link        sqlite3.so    :  16870 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2786 ms |
| RCC -O1   |      2199 ms |
| RCC -O2   |      2094 ms |
| TCC       |       359 ms |
| GCC -O0   |      2669 ms |
| GCC -O2   |     18985 ms |
| Clang -O0 |      2155 ms |
| Clang -O2 |     17327 ms |
