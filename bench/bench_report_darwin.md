# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           94 |          820 |        914 |
| RCC -O1   |           84 |          876 |        960 |
| RCC -O2   |          134 |          836 |        970 |
| TCC       |           91 |          739 |        830 |
| GCC -O0   |          106 |          621 |        727 |
| GCC -O2   |          166 |          377 |        543 |
| Clang -O0 |          112 |          572 |        684 |
| Clang -O2 |          152 |          361 |        513 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1004 us
  parse       bench.c       :    184 us
  typecheck   bench.c       :      6 us
  codegen     bench.c       :    161 us
  link        bench_rcc     :    274 us
  link        bench_rcc     :  71589 us

RCC -O1:
  preprocess  bench.c       :    794 us
  parse       bench.c       :    151 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     23 us
  codegen     bench.c       :    162 us
  link        bench_o1      :    168 us
  link        bench_o1      :  74001 us

RCC -O2:
  preprocess  bench.c       :    830 us
  parse       bench.c       :    141 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     24 us
  codegen     bench.c       :    130 us
  link        bench_o2      :    408 us
  link        bench_o2      :  80736 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 477891 us
  parse       sqlite3.c     : 187640 us
  typecheck   sqlite3.c     :  35763 us
  codegen     sqlite3.c     : 199321 us
  link        sqlite3.so    :  18251 us

RCC -O1:
  preprocess  sqlite3.c     : 420106 us
  parse       sqlite3.c     :  61999 us
  typecheck   sqlite3.c     :  16158 us
  opt         sqlite3.c     : 229679 us
  codegen     sqlite3.c     : 163799 us
  link        sqlite3.so    :  23300 us

RCC -O2:
  preprocess  sqlite3.c     : 336739 us
  parse       sqlite3.c     :  80796 us
  typecheck   sqlite3.c     :  19627 us
  opt         sqlite3.c     : 264871 us
  codegen     sqlite3.c     : 172283 us
  link        sqlite3.so    :  19536 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1212 ms |
| RCC -O1   |      1319 ms |
| RCC -O2   |      1291 ms |
| TCC       |       167 ms |
| GCC -O0   |      1931 ms |
| GCC -O2   |     14551 ms |
| Clang -O0 |      1715 ms |
| Clang -O2 |     14780 ms |
