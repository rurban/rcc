# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          124 |          766 |        890 |
| RCC -O1   |           77 |          758 |        835 |
| RCC -O2   |          108 |          787 |        895 |
| TCC       |          125 |          644 |        769 |
| GCC -O0   |          162 |          590 |        752 |
| GCC -O2   |          187 |          358 |        545 |
| Clang -O0 |           94 |          646 |        740 |
| Clang -O2 |          170 |          339 |        509 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    904 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    151 us
  link        bench_rcc     :    356 us
  link        bench_rcc     :  72906 us

RCC -O1:
  preprocess  bench.c       :    678 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    139 us
  link        bench_o1      :    168 us
  link        bench_o1      :  70649 us

RCC -O2:
  preprocess  bench.c       :    672 us
  parse       bench.c       :    143 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     17 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    641 us
  link        bench_o2      :  66580 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 479941 us
  parse       sqlite3.c     : 245034 us
  typecheck   sqlite3.c     :  22132 us
  codegen     sqlite3.c     : 233227 us
  link        sqlite3.so    :  28662 us

RCC -O1:
  preprocess  sqlite3.c     : 368789 us
  parse       sqlite3.c     :  72263 us
  typecheck   sqlite3.c     :  23698 us
  opt         sqlite3.c     :  38260 us
  codegen     sqlite3.c     : 168596 us
  link        sqlite3.so    :  23855 us

RCC -O2:
  preprocess  sqlite3.c     : 357944 us
  parse       sqlite3.c     :  76735 us
  typecheck   sqlite3.c     :  21269 us
  opt         sqlite3.c     : 300804 us
  codegen     sqlite3.c     : 156684 us
  link        sqlite3.so    :  21670 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1472 ms |
| RCC -O1   |      1101 ms |
| RCC -O2   |      1286 ms |
| TCC       |       149 ms |
| GCC -O0   |      2093 ms |
| GCC -O2   |     19131 ms |
| Clang -O0 |      1920 ms |
| Clang -O2 |     21462 ms |
