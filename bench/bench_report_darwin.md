# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          634 |        691 |
| RCC -O1   |           63 |          628 |        691 |
| RCC -O2   |           58 |          634 |        692 |
| TCC       |           47 |          554 |        601 |
| GCC -O0   |           75 |          470 |        545 |
| GCC -O2   |          114 |          284 |        398 |
| Clang -O0 |           64 |          466 |        530 |
| Clang -O2 |           93 |          284 |        377 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    919 us
  parse       bench.c       :    172 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    160 us
  link        bench_rcc     :     88 us
  link        bench_rcc     :  63644 us

RCC -O1:
  preprocess  bench.c       :    807 us
  parse       bench.c       :    203 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    134 us
  link        bench_o1      :    108 us
  link        bench_o1      :  50961 us

RCC -O2:
  preprocess  bench.c       :    857 us
  parse       bench.c       :    142 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     26 us
  codegen     bench.c       :    122 us
  link        bench_o2      :    147 us
  link        bench_o2      :  54289 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 235708 us
  parse       sqlite3.c     :  49457 us
  typecheck   sqlite3.c     :  12035 us
  codegen     sqlite3.c     :  94996 us
  link        sqlite3.so    :  14792 us

RCC -O1:
  preprocess  sqlite3.c     : 232842 us
  parse       sqlite3.c     :  48918 us
  typecheck   sqlite3.c     :  13728 us
  opt         sqlite3.c     :  19782 us
  codegen     sqlite3.c     :  95996 us
  link        sqlite3.so    :  19274 us

RCC -O2:
  preprocess  sqlite3.c     : 224170 us
  parse       sqlite3.c     :  50022 us
  typecheck   sqlite3.c     :  13829 us
  opt         sqlite3.c     : 146734 us
  codegen     sqlite3.c     :  99095 us
  link        sqlite3.so    :  15668 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       614 ms |
| RCC -O1   |       598 ms |
| RCC -O2   |       770 ms |
| TCC       |       116 ms |
| GCC -O0   |      1112 ms |
| GCC -O2   |     10294 ms |
| Clang -O0 |      1022 ms |
| Clang -O2 |      9745 ms |
