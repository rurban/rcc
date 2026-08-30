# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          696 |        784 |
| RCC -O1   |          104 |          746 |        850 |
| RCC -O2   |          210 |          775 |        985 |
| TCC       |           75 |          691 |        766 |
| GCC -O0   |          187 |          786 |        973 |
| GCC -O2   |          251 |          369 |        620 |
| Clang -O0 |           99 |          628 |        727 |
| Clang -O2 |          172 |          370 |        542 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    799 us
  parse       bench.c       :    166 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    175 us
  link        bench_rcc     :    128 us
  link        bench_rcc     :  78079 us

RCC -O1:
  preprocess  bench.c       :    710 us
  parse       bench.c       :    160 us
  typecheck   bench.c       :      3 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    304 us
  link        bench_o1      :    144 us
  link        bench_o1      :  61410 us

RCC -O2:
  preprocess  bench.c       :    799 us
  parse       bench.c       :    207 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    163 us
  link        bench_o2      :    310 us
  link        bench_o2      :  58362 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 339076 us
  parse       sqlite3.c     : 136694 us
  typecheck   sqlite3.c     :  28565 us
  codegen     sqlite3.c     : 122368 us
  link        sqlite3.so    :  18593 us

RCC -O1:
  preprocess  sqlite3.c     : 267711 us
  parse       sqlite3.c     :  62490 us
  typecheck   sqlite3.c     :  13008 us
  opt         sqlite3.c     : 172156 us
  codegen     sqlite3.c     : 116364 us
  link        sqlite3.so    :  21584 us

RCC -O2:
  preprocess  sqlite3.c     : 240735 us
  parse       sqlite3.c     :  56415 us
  typecheck   sqlite3.c     :  12295 us
  opt         sqlite3.c     : 212450 us
  codegen     sqlite3.c     : 141984 us
  link        sqlite3.so    :  21992 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1407 ms |
| RCC -O1   |       879 ms |
| RCC -O2   |       887 ms |
| TCC       |       122 ms |
| GCC -O0   |      1157 ms |
| GCC -O2   |     12459 ms |
| Clang -O0 |      1690 ms |
| Clang -O2 |     14794 ms |
