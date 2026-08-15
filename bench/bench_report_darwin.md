# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          596 |        653 |
| RCC -O1   |           47 |          592 |        639 |
| RCC -O2   |           52 |          589 |        641 |
| TCC       |           44 |          514 |        558 |
| GCC -O0   |           59 |          436 |        495 |
| GCC -O2   |           86 |          264 |        350 |
| Clang -O0 |           56 |          434 |        490 |
| Clang -O2 |           79 |          263 |        342 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    645 us
  parse       bench.c       :    124 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    117 us
  link        bench_rcc     :     85 us
  link        bench_rcc     :  47249 us

RCC -O1:
  preprocess  bench.c       :    602 us
  parse       bench.c       :    120 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    114 us
  link        bench_o1      :    159 us
  link        bench_o1      :  51018 us

RCC -O2:
  preprocess  bench.c       :    621 us
  parse       bench.c       :    123 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     19 us
  codegen     bench.c       :    151 us
  link        bench_o2      :    329 us
  link        bench_o2      :  51980 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 230026 us
  parse       sqlite3.c     :  53717 us
  typecheck   sqlite3.c     :  15813 us
  codegen     sqlite3.c     :  95168 us
  link        sqlite3.so    :  19954 us

RCC -O1:
  preprocess  sqlite3.c     : 180863 us
  parse       sqlite3.c     :  43852 us
  typecheck   sqlite3.c     :  12372 us
  opt         sqlite3.c     : 137876 us
  codegen     sqlite3.c     : 103099 us
  link        sqlite3.so    :  15797 us

RCC -O2:
  preprocess  sqlite3.c     : 195205 us
  parse       sqlite3.c     :  50883 us
  typecheck   sqlite3.c     :  12304 us
  opt         sqlite3.c     : 138266 us
  codegen     sqlite3.c     :  92825 us
  link        sqlite3.so    :  13961 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       520 ms |
| RCC -O1   |       693 ms |
| RCC -O2   |       728 ms |
| TCC       |        96 ms |
| GCC -O0   |       963 ms |
| GCC -O2   |      8979 ms |
| Clang -O0 |       946 ms |
| Clang -O2 |      8590 ms |
