# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          115 |          742 |        857 |
| RCC -O1   |          107 |          744 |        851 |
| RCC -O2   |           75 |          731 |        806 |
| TCC       |           56 |          671 |        727 |
| GCC -O0   |          130 |          549 |        679 |
| GCC -O2   |          237 |          326 |        563 |
| Clang -O0 |          110 |          542 |        652 |
| Clang -O2 |          146 |          309 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1550 us
  parse       bench.c:    284 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    273 us
  link        bench_rcc: 111020 us

RCC -O1:
  preprocess  bench.c:    736 us
  parse       bench.c:    259 us
  typecheck   bench.c:      9 us
  opt         bench.c:     43 us
  codegen     bench.c:    271 us
  link        bench_rcc_o1:  67888 us

RCC -O2:
  preprocess  bench.c:    670 us
  parse       bench.c:    187 us
  typecheck   bench.c:      6 us
  opt         bench.c:     22 us
  codegen     bench.c:    125 us
  link        bench_rcc_o2:  71583 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 340562 us
  parse       sqlite3.c:  64586 us
  typecheck   sqlite3.c:  25993 us
  codegen     sqlite3.c:  85450 us

RCC -O1:
  preprocess  sqlite3.c: 290495 us
  parse       sqlite3.c:  70339 us
  typecheck   sqlite3.c:  18100 us
  opt         sqlite3.c:  21192 us
  codegen     sqlite3.c:  76626 us

RCC -O2:
  preprocess  sqlite3.c: 333304 us
  parse       sqlite3.c:  53576 us
  typecheck   sqlite3.c:  20309 us
  opt         sqlite3.c: 238766 us
  codegen     sqlite3.c:  75381 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       836 ms |
| RCC -O1   |       731 ms |
| RCC -O2   |       750 ms |
| TCC       |        96 ms |
| GCC -O0   |      1161 ms |
| GCC -O2   |     12292 ms |
| Clang -O0 |      1271 ms |
| Clang -O2 |     12208 ms |
