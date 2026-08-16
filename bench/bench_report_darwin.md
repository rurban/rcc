# Darwin RCC Benchmark Results

_Generated: August 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          125 |          714 |        839 |
| RCC -O1   |           74 |          706 |        780 |
| RCC -O2   |          112 |          652 |        764 |
| TCC       |           45 |          601 |        646 |
| GCC -O0   |           87 |          615 |        702 |
| GCC -O2   |          162 |          339 |        501 |
| Clang -O0 |           86 |          602 |        688 |
| Clang -O2 |          196 |          364 |        560 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :    784 us
  parse       bench.c       :    135 us
  typecheck   bench.c       :      5 us
  codegen     bench.c       :    128 us
  link        bench_rcc     :    435 us
  link        bench_rcc     :  65319 us

RCC -O1:
  preprocess  bench.c       :    637 us
  parse       bench.c       :    136 us
  typecheck   bench.c       :      6 us
  opt         bench.c       :     21 us
  codegen     bench.c       :    121 us
  link        bench_o1      :    443 us
  link        bench_o1      :  67058 us

RCC -O2:
  preprocess  bench.c       :    656 us
  parse       bench.c       :    139 us
  typecheck   bench.c       :      5 us
  opt         bench.c       :     22 us
  codegen     bench.c       :    128 us
  link        bench_o2      :    402 us
  link        bench_o2      :  62781 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 399506 us
  parse       sqlite3.c     :  85480 us
  typecheck   sqlite3.c     :  20719 us
  codegen     sqlite3.c     : 134700 us
  link        sqlite3.so    :  22243 us

RCC -O1:
  preprocess  sqlite3.c     : 302099 us
  parse       sqlite3.c     :  64225 us
  typecheck   sqlite3.c     :  19581 us
  opt         sqlite3.c     : 244719 us
  codegen     sqlite3.c     : 126087 us
  link        sqlite3.so    :  17061 us

RCC -O2:
  preprocess  sqlite3.c     : 283467 us
  parse       sqlite3.c     :  60610 us
  typecheck   sqlite3.c     :  16704 us
  opt         sqlite3.c     : 155079 us
  codegen     sqlite3.c     : 154097 us
  link        sqlite3.so    :  25094 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1685 ms |
| RCC -O1   |      1182 ms |
| RCC -O2   |       951 ms |
| TCC       |       158 ms |
| GCC -O0   |      1540 ms |
| GCC -O2   |     13714 ms |
| Clang -O0 |      1322 ms |
| Clang -O2 |     13298 ms |
