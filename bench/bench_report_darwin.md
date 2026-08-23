# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           90 |          798 |        888 |
| RCC -O1   |           77 |          827 |        904 |
| RCC -O2   |          116 |          800 |        916 |
| TCC       |           58 |          603 |        661 |
| GCC -O0   |           96 |          651 |        747 |
| GCC -O2   |          155 |          355 |        510 |
| Clang -O0 |           86 |          577 |        663 |
| Clang -O2 |          120 |          365 |        485 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   1981 us
  parse       bench.c       :    347 us
  typecheck   bench.c       :     10 us
  codegen     bench.c       :    278 us
  link        bench_rcc     :    157 us
  link        bench_rcc     :  72314 us

RCC -O1:
  preprocess  bench.c       :    674 us
  parse       bench.c       :    179 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    131 us
  link        bench_o1      :    223 us
  link        bench_o1      :  61463 us

RCC -O2:
  preprocess  bench.c       :    666 us
  parse       bench.c       :    129 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    129 us
  link        bench_o2      :    141 us
  link        bench_o2      :  55941 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 328002 us
  parse       sqlite3.c     : 196855 us
  typecheck   sqlite3.c     :  25559 us
  codegen     sqlite3.c     : 128129 us
  link        sqlite3.so    :  15951 us

RCC -O1:
  preprocess  sqlite3.c     : 299995 us
  parse       sqlite3.c     :  70111 us
  typecheck   sqlite3.c     :  13542 us
  opt         sqlite3.c     : 182043 us
  codegen     sqlite3.c     : 159585 us
  link        sqlite3.so    :  19317 us

RCC -O2:
  preprocess  sqlite3.c     : 305894 us
  parse       sqlite3.c     :  64920 us
  typecheck   sqlite3.c     :  16058 us
  opt         sqlite3.c     : 185173 us
  codegen     sqlite3.c     : 168336 us
  link        sqlite3.so    :  22580 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1296 ms |
| RCC -O1   |      1266 ms |
| RCC -O2   |      1087 ms |
| TCC       |       119 ms |
| GCC -O0   |      1236 ms |
| GCC -O2   |     14416 ms |
| Clang -O0 |      1785 ms |
| Clang -O2 |     14170 ms |
