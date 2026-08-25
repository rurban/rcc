# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          694 |        749 |
| RCC -O1   |           73 |          684 |        757 |
| RCC -O2   |           66 |          661 |        727 |
| TCC       |           40 |          518 |        558 |
| GCC -O0   |           73 |          434 |        507 |
| GCC -O2   |           89 |          263 |        352 |
| Clang -O0 |           51 |          434 |        485 |
| Clang -O2 |           79 |          265 |        344 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    599 us
  parse       bench.c       :    121 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    110 us
  link        bench_rcc     :     97 us
  link        bench_rcc     :  45319 us

RCC -O1:
  preprocess  bench.c       :    580 us
  parse       bench.c       :    128 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    113 us
  link        bench_o1      :    152 us
  link        bench_o1      :  44543 us

RCC -O2:
  preprocess  bench.c       :    563 us
  parse       bench.c       :    122 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     20 us
  codegen     bench.c       :    113 us
  link        bench_o2      :    184 us
  link        bench_o2      :  44790 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 175209 us
  parse       sqlite3.c     :  44011 us
  typecheck   sqlite3.c     :  11639 us
  codegen     sqlite3.c     :  84625 us
  link        sqlite3.so    :  13984 us

RCC -O1:
  preprocess  sqlite3.c     : 172260 us
  parse       sqlite3.c     :  44153 us
  typecheck   sqlite3.c     :  11726 us
  opt         sqlite3.c     : 119311 us
  codegen     sqlite3.c     :  86680 us
  link        sqlite3.so    :  13853 us

RCC -O2:
  preprocess  sqlite3.c     : 172073 us
  parse       sqlite3.c     :  44068 us
  typecheck   sqlite3.c     :  11738 us
  opt         sqlite3.c     : 122485 us
  codegen     sqlite3.c     :  87596 us
  link        sqlite3.so    :  14334 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       659 ms |
| RCC -O1   |       641 ms |
| RCC -O2   |       639 ms |
| TCC       |        92 ms |
| GCC -O0   |       950 ms |
| GCC -O2   |     10506 ms |
| Clang -O0 |      1021 ms |
| Clang -O2 |      8710 ms |
