# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          146 |          790 |        936 |
| RCC -O1   |           84 |          731 |        815 |
| RCC -O2   |           79 |          711 |        790 |
| TCC       |           69 |          568 |        637 |
| GCC -O0   |          102 |          466 |        568 |
| GCC -O2   |          130 |          285 |        415 |
| Clang -O0 |           70 |          510 |        580 |
| Clang -O2 |          161 |          325 |        486 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1575 us
  parse       bench.c:    205 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    129 us
  link        bench_rcc:  72280 us

RCC -O1:
  preprocess  bench.c:    627 us
  parse       bench.c:    123 us
  typecheck   bench.c:      4 us
  opt         bench.c:     17 us
  codegen     bench.c:    117 us
  link        bench_rcc_o1:  83944 us

RCC -O2:
  preprocess  bench.c:    618 us
  parse       bench.c:    117 us
  typecheck   bench.c:      4 us
  opt         bench.c:     17 us
  codegen     bench.c:    110 us
  link        bench_rcc_o2:  90515 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 325213 us
  parse       sqlite3.c:  52651 us
  typecheck   sqlite3.c:  18866 us
  codegen     sqlite3.c:  53119 us

RCC -O1:
  preprocess  sqlite3.c: 240055 us
  parse       sqlite3.c:  46040 us
  typecheck   sqlite3.c:  13022 us
  opt         sqlite3.c:  19993 us
  codegen     sqlite3.c:  49605 us

RCC -O2:
  preprocess  sqlite3.c: 275656 us
  parse       sqlite3.c:  53267 us
  typecheck   sqlite3.c:  16764 us
  opt         sqlite3.c: 163508 us
  codegen     sqlite3.c:  53545 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       669 ms |
| RCC -O1   |       440 ms |
| RCC -O2   |       574 ms |
| TCC       |        81 ms |
| GCC -O0   |      1088 ms |
| GCC -O2   |     13181 ms |
| Clang -O0 |      1494 ms |
| Clang -O2 |     13324 ms |
