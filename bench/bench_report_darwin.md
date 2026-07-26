# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           69 |          688 |        757 |
| RCC -O1   |           56 |          629 |        685 |
| RCC -O2   |           56 |          609 |        665 |
| TCC       |           52 |          603 |        655 |
| GCC -O0   |           72 |          508 |        580 |
| GCC -O2   |          127 |          271 |        398 |
| Clang -O0 |           53 |          470 |        523 |
| Clang -O2 |           95 |          290 |        385 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    995 us
  parse       bench.c:    169 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    205 us
  link        bench_rcc:  65307 us

RCC -O1:
  preprocess  bench.c:    720 us
  parse       bench.c:    149 us
  typecheck   bench.c:      5 us
  opt         bench.c:     25 us
  codegen     bench.c:    125 us
  link        bench_rcc_o1:  63550 us

RCC -O2:
  preprocess  bench.c:    590 us
  parse       bench.c:    129 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    148 us
  link        bench_rcc_o2:  62319 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 335871 us
  parse       sqlite3.c:  85653 us
  typecheck   sqlite3.c:  17356 us
  codegen     sqlite3.c:  52833 us

RCC -O1:
  preprocess  sqlite3.c: 221908 us
  parse       sqlite3.c:  43289 us
  typecheck   sqlite3.c:  12198 us
  opt         sqlite3.c:  19917 us
  codegen     sqlite3.c:  45388 us

RCC -O2:
  preprocess  sqlite3.c: 212164 us
  parse       sqlite3.c:  46928 us
  typecheck   sqlite3.c:  11264 us
  opt         sqlite3.c: 144222 us
  codegen     sqlite3.c:  48369 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       568 ms |
| RCC -O1   |       645 ms |
| RCC -O2   |       750 ms |
| TCC       |        95 ms |
| GCC -O0   |      1416 ms |
| GCC -O2   |     11536 ms |
| Clang -O0 |      1291 ms |
| Clang -O2 |     11555 ms |
