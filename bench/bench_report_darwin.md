# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           80 |          704 |        784 |
| RCC -O1   |           75 |          654 |        729 |
| RCC -O2   |           85 |          644 |        729 |
| TCC       |           63 |          597 |        660 |
| GCC -O0   |          104 |          499 |        603 |
| GCC -O2   |          150 |          313 |        463 |
| Clang -O0 |          100 |          609 |        709 |
| Clang -O2 |          215 |          346 |        561 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1055 us
  parse       bench.c:    169 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    139 us
  native link bench_rcc:      1 us
  link        bench_rcc:  65215 us

RCC -O1:
  preprocess  bench.c:    656 us
  parse       bench.c:    151 us
  typecheck   bench.c:      5 us
  opt         bench.c:     25 us
  codegen     bench.c:    154 us
  native link bench_rcc_o1:      0 us
  link        bench_rcc_o1:  65993 us

RCC -O2:
  preprocess  bench.c:    737 us
  parse       bench.c:    153 us
  typecheck   bench.c:      5 us
  opt         bench.c:     30 us
  codegen     bench.c:    150 us
  native link bench_rcc_o2:      1 us
  link        bench_rcc_o2:  64114 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 322935 us
  parse       sqlite3.c: 113505 us
  typecheck   sqlite3.c:  17946 us
  codegen     sqlite3.c:  56017 us

RCC -O1:
  preprocess  sqlite3.c: 250853 us
  parse       sqlite3.c:  48628 us
  typecheck   sqlite3.c:  12170 us
  opt         sqlite3.c:  19491 us
  codegen     sqlite3.c:  49430 us

RCC -O2:
  preprocess  sqlite3.c: 292416 us
  parse       sqlite3.c:  60993 us
  typecheck   sqlite3.c:  16557 us
  opt         sqlite3.c: 209421 us
  codegen     sqlite3.c:  61825 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1049 ms |
| RCC -O1   |       768 ms |
| RCC -O2   |       899 ms |
| TCC       |       103 ms |
| GCC -O0   |      1271 ms |
| GCC -O2   |     11171 ms |
| Clang -O0 |      1220 ms |
| Clang -O2 |     11951 ms |
