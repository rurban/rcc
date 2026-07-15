# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           57 |          743 |        800 |
| RCC -O1   |          100 |          758 |        858 |
| TCC       |           76 |          594 |        670 |
| GCC -O0   |          140 |          481 |        621 |
| GCC -O2   |          139 |          293 |        432 |
| Clang -O0 |           72 |          474 |        546 |
| Clang -O2 |          108 |          298 |        406 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    558 us
  lex         bench.c:     71 us
  parse       bench.c:    135 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    134 us
  link        bench_rcc:  50017 us

RCC -O1:
  preprocess  bench.c:    503 us
  lex         bench.c:     82 us
  parse       bench.c:    118 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    247 us
  link        bench_rcc_o1:  50819 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       123 ms |
| GCC -O0   |      1141 ms |
| GCC -O2   |     13432 ms |
| Clang -O0 |      1510 ms |
| Clang -O2 |     10666 ms |
