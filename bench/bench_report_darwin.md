# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          137 |          790 |        927 |
| RCC -O1   |          103 |          666 |        769 |
| RCC -O2   |           85 |          723 |        808 |
| TCC       |           52 |          649 |        701 |
| GCC -O0   |          156 |          582 |        738 |
| GCC -O2   |          193 |          306 |        499 |
| Clang -O0 |          113 |          563 |        676 |
| Clang -O2 |          185 |          359 |        544 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2004 us
  parse       bench.c:    168 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    160 us
  link        bench_rcc:  66478 us

RCC -O1:
  preprocess  bench.c:    610 us
  parse       bench.c:    119 us
  typecheck   bench.c:      5 us
  opt         bench.c:     23 us
  codegen     bench.c:    131 us
  link        bench_rcc_o1:  66041 us

RCC -O2:
  preprocess  bench.c:    645 us
  parse       bench.c:    129 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    119 us
  link        bench_rcc_o2:  69024 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 561443 us
  parse       sqlite3.c: 254398 us
  typecheck   sqlite3.c:  31609 us
  codegen     sqlite3.c: 107878 us

RCC -O1:
  preprocess  sqlite3.c: 396488 us
  parse       sqlite3.c:  78530 us
  typecheck   sqlite3.c:  21267 us
  opt         sqlite3.c:  32233 us
  codegen     sqlite3.c:  76791 us

RCC -O2:
  preprocess  sqlite3.c: 325516 us
  parse       sqlite3.c:  66259 us
  typecheck   sqlite3.c:  16779 us
  opt         sqlite3.c: 297319 us
  codegen     sqlite3.c: 132874 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       965 ms |
| RCC -O1   |       557 ms |
| RCC -O2   |       988 ms |
| TCC       |       185 ms |
| GCC -O0   |      1631 ms |
| GCC -O2   |     13502 ms |
| Clang -O0 |      1306 ms |
| Clang -O2 |     11793 ms |
