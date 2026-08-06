# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           49 |          586 |        635 |
| RCC -O1   |           50 |          584 |        634 |
| RCC -O2   |           47 |          582 |        629 |
| TCC       |           39 |          513 |        552 |
| GCC -O0   |           57 |          435 |        492 |
| GCC -O2   |          106 |          264 |        370 |
| Clang -O0 |           59 |          433 |        492 |
| Clang -O2 |           79 |          262 |        341 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    590 us
  parse       bench.c       :    110 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    121 us
  link        bench_rcc     :     81 us
  link        bench_rcc     :  43637 us

RCC -O1:
  preprocess  bench.c       :    871 us
  parse       bench.c       :    145 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    205 us
  link        bench_o1      :    112 us
  link        bench_o1      :  44446 us

RCC -O2:
  preprocess  bench.c       :    538 us
  parse       bench.c       :    118 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     18 us
  codegen     bench.c       :    105 us
  link        bench_o2      :    182 us
  link        bench_o2      :  43501 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 224157 us
  parse       sqlite3.c     :  42747 us
  typecheck   sqlite3.c     :  12060 us
  codegen     sqlite3.c     :  86950 us
  link        sqlite3.so    :  14435 us

RCC -O1:
  preprocess  sqlite3.c     : 190187 us
  parse       sqlite3.c     :  39972 us
  typecheck   sqlite3.c     :  11873 us
  opt         sqlite3.c     :  18412 us
  codegen     sqlite3.c     :  84708 us
  link        sqlite3.so    :  13343 us

RCC -O2:
  preprocess  sqlite3.c     : 210456 us
  parse       sqlite3.c     :  41227 us
  typecheck   sqlite3.c     :  13099 us
  opt         sqlite3.c     : 126832 us
  codegen     sqlite3.c     :  86374 us
  link        sqlite3.so    :  16050 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       513 ms |
| RCC -O1   |       535 ms |
| RCC -O2   |       634 ms |
| TCC       |        91 ms |
| GCC -O0   |       936 ms |
| GCC -O2   |      9009 ms |
| Clang -O0 |       912 ms |
| Clang -O2 |      8751 ms |
