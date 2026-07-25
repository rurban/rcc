# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          619 |        671 |
| RCC -O1   |           49 |          581 |        630 |
| RCC -O2   |           49 |          579 |        628 |
| TCC       |           39 |          514 |        553 |
| GCC -O0   |           63 |          436 |        499 |
| GCC -O2   |          158 |          309 |        467 |
| Clang -O0 |           80 |          475 |        555 |
| Clang -O2 |          112 |          289 |        401 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    982 us
  parse       bench.c:    137 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    152 us
  link        bench_rcc:  47021 us

RCC -O1:
  preprocess  bench.c:    512 us
  parse       bench.c:    140 us
  typecheck   bench.c:      5 us
  opt         bench.c:     36 us
  codegen     bench.c:    135 us
  link        bench_rcc_o1:  45039 us

RCC -O2:
  preprocess  bench.c:    620 us
  parse       bench.c:    120 us
  typecheck   bench.c:      4 us
  opt         bench.c:     20 us
  codegen     bench.c:    115 us
  link        bench_rcc_o2:  43978 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 218340 us
  parse       sqlite3.c:  39617 us
  typecheck   sqlite3.c:  11209 us
  codegen     sqlite3.c:  40233 us

RCC -O1:
  preprocess  sqlite3.c: 235018 us
  parse       sqlite3.c:  55076 us
  typecheck   sqlite3.c:  12373 us
  opt         sqlite3.c:  19561 us
  codegen     sqlite3.c:  44083 us

RCC -O2:
  preprocess  sqlite3.c: 185276 us
  parse       sqlite3.c:  40396 us
  typecheck   sqlite3.c:  11021 us
  opt         sqlite3.c: 117052 us
  codegen     sqlite3.c:  41719 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       727 ms |
| RCC -O1   |       770 ms |
| RCC -O2   |       705 ms |
| TCC       |        71 ms |
| GCC -O0   |       988 ms |
| GCC -O2   |      9119 ms |
| Clang -O0 |       907 ms |
| Clang -O2 |      8644 ms |
