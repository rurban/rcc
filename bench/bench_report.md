# Linux RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           16 |          871 |        887 |
| RCC -O1   |           17 |          874 |        891 |
| RCC -O2   |           16 |          849 |        865 |
| TCC       |           11 |          620 |        631 |
| KEFIR     |          278 |          757 |       1035 |
| KEFIR -O1 |          272 |          435 |        707 |
| SCC       |           50 |          766 |        816 |
| LACC      |           32 |          956 |        988 |
| CANTCC    |           37 |          539 |        576 |
| CCC       |           49 |          726 |        775 |
| GCC -O0   |           80 |          656 |        736 |
| GCC -O2   |          228 |          224 |        452 |
| Clang -O0 |          108 |          643 |        751 |
| Clang -O2 |          176 |          234 |        410 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   4960 us
  parse       bench.c       :    418 us
  typecheck   bench.c       :     24 us
  codegen     bench.c       :    524 us
  link        bench_rcc     :    454 us

RCC -O1:
  preprocess  bench.c       :   5401 us
  parse       bench.c       :    543 us
  typecheck   bench.c       :      9 us
  opt         bench.c       :     37 us
  codegen     bench.c       :    440 us
  link        bench_o1      :    598 us

RCC -O2:
  preprocess  bench.c       :   6065 us
  parse       bench.c       :    672 us
  typecheck   bench.c       :      8 us
  opt         bench.c       :     35 us
  codegen     bench.c       :    466 us
  link        bench_o2      :    534 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 348611 us
  parse       sqlite3.c     : 156562 us
  typecheck   sqlite3.c     :  12136 us
  codegen     sqlite3.c     : 221892 us
  link        sqlite3.so    :   8379 us

RCC -O1:
  preprocess  sqlite3.c     : 286028 us
  parse       sqlite3.c     : 158540 us
  typecheck   sqlite3.c     :  10092 us
  opt         sqlite3.c     :  37794 us
  codegen     sqlite3.c     : 239222 us
  link        sqlite3.so    :   8398 us

RCC -O2:
  preprocess  sqlite3.c     : 287563 us
  parse       sqlite3.c     : 156783 us
  typecheck   sqlite3.c     :  14137 us
  opt         sqlite3.c     : 281123 us
  codegen     sqlite3.c     : 252267 us
  link        sqlite3.so    :  10391 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1081 ms |
| RCC -O1   |      1172 ms |
| RCC -O2   |      1394 ms |
| TCC       |       149 ms |
| KEFIR     |     27165 ms |
| KEFIR -O1 |     51316 ms |
| ANTCC     |       547 ms |
| CCC       |     17808 ms |
| GCC -O0   |      5834 ms |
| GCC -O2   |     36813 ms |
| Clang -O0 |      2388 ms |
| Clang -O2 |     28759 ms |
