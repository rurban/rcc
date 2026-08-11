# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          104 |          766 |        870 |
| RCC -O1   |           68 |          702 |        770 |
| RCC -O2   |           78 |          785 |        863 |
| TCC       |          237 |          666 |        903 |
| GCC -O0   |          123 |          528 |        651 |
| GCC -O2   |          141 |          306 |        447 |
| Clang -O0 |           64 |          547 |        611 |
| Clang -O2 |          150 |          367 |        517 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    924 us
  parse       bench.c       :    309 us
  typecheck   bench.c       :     13 us
  codegen     bench.c       :    154 us
  link        bench_rcc     :    456 us
  link        bench_rcc     :  82238 us

RCC -O1:
  preprocess  bench.c       :   2181 us
  parse       bench.c       :    447 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    146 us
  link        bench_o1      :    471 us
  link        bench_o1      :  68272 us

RCC -O2:
  preprocess  bench.c       :    786 us
  parse       bench.c       :    138 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    238 us
  link        bench_o2      :    389 us
  link        bench_o2      :  78699 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 555364 us
  parse       sqlite3.c     : 247800 us
  typecheck   sqlite3.c     :  18849 us
  codegen     sqlite3.c     : 183828 us
  link        sqlite3.so    :  23806 us

RCC -O1:
  preprocess  sqlite3.c     : 432356 us
  parse       sqlite3.c     :  74121 us
  typecheck   sqlite3.c     :  22570 us
  opt         sqlite3.c     :  46437 us
  codegen     sqlite3.c     : 138756 us
  link        sqlite3.so    :  15936 us

RCC -O2:
  preprocess  sqlite3.c     : 318123 us
  parse       sqlite3.c     :  60028 us
  typecheck   sqlite3.c     :  19449 us
  opt         sqlite3.c     : 174031 us
  codegen     sqlite3.c     : 109167 us
  link        sqlite3.so    :  17386 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      2041 ms |
| RCC -O1   |      1028 ms |
| RCC -O2   |      1029 ms |
| TCC       |       216 ms |
| GCC -O0   |      1621 ms |
| GCC -O2   |     14831 ms |
| Clang -O0 |      1608 ms |
| Clang -O2 |     12881 ms |
