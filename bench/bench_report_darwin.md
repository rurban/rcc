# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          673 |        727 |
| RCC -O1   |           53 |          626 |        679 |
| RCC -O2   |           62 |          640 |        702 |
| TCC       |           45 |          562 |        607 |
| GCC -O0   |           82 |          514 |        596 |
| GCC -O2   |          132 |          288 |        420 |
| Clang -O0 |           62 |          473 |        535 |
| Clang -O2 |           99 |          288 |        387 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    754 us
  parse       bench.c:    152 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    155 us
  link        bench_rcc:  69945 us

RCC -O1:
  preprocess  bench.c:    610 us
  parse       bench.c:    115 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    151 us
  link        bench_rcc_o1:  53552 us

RCC -O2:
  preprocess  bench.c:    607 us
  parse       bench.c:    130 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    130 us
  link        bench_rcc_o2:  49088 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 323728 us
  parse       sqlite3.c:  57703 us
  typecheck   sqlite3.c:  23277 us
  codegen     sqlite3.c:  61364 us

RCC -O1:
  preprocess  sqlite3.c: 207559 us
  parse       sqlite3.c:  45754 us
  typecheck   sqlite3.c:  13517 us
  opt         sqlite3.c:  19940 us
  codegen     sqlite3.c:  47528 us

RCC -O2:
  preprocess  sqlite3.c: 207595 us
  parse       sqlite3.c:  43838 us
  typecheck   sqlite3.c:  12050 us
  opt         sqlite3.c: 133397 us
  codegen     sqlite3.c:  46091 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       559 ms |
| RCC -O1   |       376 ms |
| RCC -O2   |       545 ms |
| TCC       |        84 ms |
| GCC -O0   |      1123 ms |
| GCC -O2   |     12035 ms |
| Clang -O0 |      1161 ms |
| Clang -O2 |     11943 ms |
