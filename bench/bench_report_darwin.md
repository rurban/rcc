# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          152 |          882 |       1034 |
| RCC -O1   |           81 |          817 |        898 |
| RCC -O2   |          100 |          843 |        943 |
| TCC       |           82 |          758 |        840 |
| GCC -O0   |          123 |          667 |        790 |
| GCC -O2   |          207 |          368 |        575 |
| Clang -O0 |          116 |          649 |        765 |
| Clang -O2 |          147 |          342 |        489 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    805 us
  parse       bench.c       :    146 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    150 us
  link        bench_rcc     :     91 us
  link        bench_rcc     :  49318 us

RCC -O1:
  preprocess  bench.c       :    647 us
  parse       bench.c       :    177 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    142 us
  link        bench_o1      :    116 us
  link        bench_o1      :  57198 us

RCC -O2:
  preprocess  bench.c       :    813 us
  parse       bench.c       :    229 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     25 us
  codegen     bench.c       :    197 us
  link        bench_o2      :    170 us
  link        bench_o2      :  70202 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 352825 us
  parse       sqlite3.c     :  62166 us
  typecheck   sqlite3.c     :  11812 us
  codegen     sqlite3.c     : 117355 us
  link        sqlite3.so    :  21242 us

RCC -O1:
  preprocess  sqlite3.c     : 436829 us
  parse       sqlite3.c     :  84739 us
  typecheck   sqlite3.c     :  21377 us
  opt         sqlite3.c     : 249313 us
  codegen     sqlite3.c     : 184518 us
  link        sqlite3.so    :  31898 us

RCC -O2:
  preprocess  sqlite3.c     : 356836 us
  parse       sqlite3.c     :  91311 us
  typecheck   sqlite3.c     :  21486 us
  opt         sqlite3.c     : 342469 us
  codegen     sqlite3.c     : 182929 us
  link        sqlite3.so    :  27823 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1337 ms |
| RCC -O1   |      1254 ms |
| RCC -O2   |      1072 ms |
| TCC       |       136 ms |
| GCC -O0   |      1737 ms |
| GCC -O2   |     15361 ms |
| Clang -O0 |      1609 ms |
| Clang -O2 |     12945 ms |
