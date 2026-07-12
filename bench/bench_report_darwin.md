# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          706 |        798 |
| RCC -O1   |           85 |          679 |        764 |
| TCC       |           82 |          594 |        676 |
| GCC -O0   |          104 |          500 |        604 |
| GCC -O2   |          151 |          312 |        463 |
| Clang -O0 |           96 |          539 |        635 |
| Clang -O2 |          128 |          315 |        443 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    830 us
  lex         bench.c:    156 us
  parse       bench.c:    167 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    159 us
  link        bench_rcc:  73128 us

RCC -O1:
  preprocess  bench.c:    535 us
  lex         bench.c:     75 us
  parse       bench.c:    238 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    140 us
  link        bench_rcc_o1:  66699 us
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
| TCC       |       125 ms |
| GCC -O0   |      1118 ms |
| GCC -O2   |     14656 ms |
| Clang -O0 |      1416 ms |
| Clang -O2 |     12778 ms |
