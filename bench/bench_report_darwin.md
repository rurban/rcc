# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           76 |          799 |        875 |
| RCC -O1   |          132 |          777 |        909 |
| TCC       |           90 |          747 |        837 |
| GCC -O0   |          161 |          630 |        791 |
| GCC -O2   |          274 |          368 |        642 |
| Clang -O0 |          131 |          628 |        759 |
| Clang -O2 |          221 |          368 |        589 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    836 us
  lex         bench.c:     93 us
  parse       bench.c:    164 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    185 us
  link        bench_rcc:  82628 us

RCC -O1:
  preprocess  bench.c:    940 us
  lex         bench.c:     87 us
  parse       bench.c:    153 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    159 us
  link        bench_rcc_o1:  82244 us
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
| TCC       |       209 ms |
| GCC -O0   |      1983 ms |
| GCC -O2   |     20431 ms |
| Clang -O0 |      1746 ms |
| Clang -O2 |     14176 ms |
