# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          123 |          841 |        964 |
| RCC -O1   |           90 |          768 |        858 |
| RCC -O2   |           88 |          770 |        858 |
| TCC       |           95 |          798 |        893 |
| GCC -O0   |          132 |          576 |        708 |
| GCC -O2   |          237 |          397 |        634 |
| Clang -O0 |          123 |          632 |        755 |
| Clang -O2 |          146 |          380 |        526 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   3753 us
  parse       bench.c:    179 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    135 us
  link        bench_rcc:  76649 us

RCC -O1:
  preprocess  bench.c:   1869 us
  parse       bench.c:    281 us
  typecheck   bench.c:      9 us
  opt         bench.c:     40 us
  codegen     bench.c:    251 us
  link        bench_rcc_o1:  75935 us

RCC -O2:
  preprocess  bench.c:    667 us
  parse       bench.c:    270 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    137 us
  link        bench_rcc_o2:  73092 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 510157 us
  parse       sqlite3.c: 183206 us
  typecheck   sqlite3.c:  26369 us
  codegen     sqlite3.c: 118237 us

RCC -O1:
  preprocess  sqlite3.c: 398276 us
  parse       sqlite3.c:  74876 us
  typecheck   sqlite3.c:  30988 us
  opt         sqlite3.c:  31630 us
  codegen     sqlite3.c:  85054 us

RCC -O2:
  preprocess  sqlite3.c: 402205 us
  parse       sqlite3.c:  87018 us
  typecheck   sqlite3.c:  19421 us
  opt         sqlite3.c: 263418 us
  codegen     sqlite3.c: 182232 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       887 ms |
| RCC -O1   |       551 ms |
| RCC -O2   |       897 ms |
| TCC       |       135 ms |
| GCC -O0   |      1546 ms |
| GCC -O2   |     14817 ms |
| Clang -O0 |      1471 ms |
| Clang -O2 |     16197 ms |
