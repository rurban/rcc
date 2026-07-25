# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          113 |          770 |        883 |
| RCC -O1   |           70 |          778 |        848 |
| RCC -O2   |          152 |          735 |        887 |
| TCC       |           48 |          666 |        714 |
| GCC -O0   |          106 |          522 |        628 |
| GCC -O2   |          200 |          375 |        575 |
| Clang -O0 |           79 |          594 |        673 |
| Clang -O2 |          136 |          347 |        483 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    838 us
  parse       bench.c:    132 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    234 us
  link        bench_rcc:  68479 us

RCC -O1:
  preprocess  bench.c:    639 us
  parse       bench.c:    175 us
  typecheck   bench.c:      5 us
  opt         bench.c:     23 us
  codegen     bench.c:    170 us
  link        bench_rcc_o1:  68238 us

RCC -O2:
  preprocess  bench.c:    673 us
  parse       bench.c:    173 us
  typecheck   bench.c:      5 us
  opt         bench.c:     23 us
  codegen     bench.c:    120 us
  link        bench_rcc_o2:  71984 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 382818 us
  parse       sqlite3.c: 146931 us
  typecheck   sqlite3.c:  29887 us
  codegen     sqlite3.c: 109702 us

RCC -O1:
  preprocess  sqlite3.c: 390600 us
  parse       sqlite3.c:  60465 us
  typecheck   sqlite3.c:  18003 us
  opt         sqlite3.c:  30240 us
  codegen     sqlite3.c:  72539 us

RCC -O2:
  preprocess  sqlite3.c: 461652 us
  parse       sqlite3.c:  83773 us
  typecheck   sqlite3.c:  29748 us
  opt         sqlite3.c: 236893 us
  codegen     sqlite3.c:  90654 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1274 ms |
| RCC -O1   |       925 ms |
| RCC -O2   |      1012 ms |
| TCC       |        95 ms |
| GCC -O0   |      1833 ms |
| GCC -O2   |     14721 ms |
| Clang -O0 |      1626 ms |
| Clang -O2 |     16381 ms |
