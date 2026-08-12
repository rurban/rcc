# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          101 |          705 |        806 |
| RCC -O1   |           69 |          716 |        785 |
| RCC -O2   |           65 |          710 |        775 |
| TCC       |          149 |          777 |        926 |
| GCC -O0   |          186 |          616 |        802 |
| GCC -O2   |          149 |          358 |        507 |
| Clang -O0 |          148 |          757 |        905 |
| Clang -O2 |          244 |          365 |        609 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    806 us
  parse       bench.c       :    157 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    129 us
  link        bench_rcc     :     88 us
  link        bench_rcc     :  63657 us

RCC -O1:
  preprocess  bench.c       :    642 us
  parse       bench.c       :    158 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    136 us
  link        bench_o1      :    142 us
  link        bench_o1      :  56706 us

RCC -O2:
  preprocess  bench.c       :    807 us
  parse       bench.c       :    154 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    139 us
  link        bench_o2      :    115 us
  link        bench_o2      :  59209 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 318999 us
  parse       sqlite3.c     : 179521 us
  typecheck   sqlite3.c     :  30568 us
  codegen     sqlite3.c     : 183037 us
  link        sqlite3.so    :  17370 us

RCC -O1:
  preprocess  sqlite3.c     : 411044 us
  parse       sqlite3.c     :  66163 us
  typecheck   sqlite3.c     :  40742 us
  opt         sqlite3.c     :  27118 us
  codegen     sqlite3.c     : 112964 us
  link        sqlite3.so    :  16563 us

RCC -O2:
  preprocess  sqlite3.c     : 326464 us
  parse       sqlite3.c     :  76055 us
  typecheck   sqlite3.c     :  15658 us
  opt         sqlite3.c     : 205077 us
  codegen     sqlite3.c     : 217330 us
  link        sqlite3.so    :  29039 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1495 ms |
| RCC -O1   |       962 ms |
| RCC -O2   |      1229 ms |
| TCC       |       199 ms |
| GCC -O0   |      2245 ms |
| GCC -O2   |     16629 ms |
| Clang -O0 |      1385 ms |
| Clang -O2 |     14694 ms |
