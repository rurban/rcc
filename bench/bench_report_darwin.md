# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          746 |        799 |
| RCC -O1   |           71 |          603 |        674 |
| TCC       |           34 |          543 |        577 |
| GCC -O0   |           64 |          436 |        500 |
| GCC -O2   |          109 |          274 |        383 |
| Clang -O0 |           62 |          440 |        502 |
| Clang -O2 |           83 |          266 |        349 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    503 us
  lex         bench.c:     72 us
  parse       bench.c:    107 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    132 us
  link        bench_rcc:  47245 us

RCC -O1:
  preprocess  bench.c:    440 us
  lex         bench.c:     70 us
  parse       bench.c:    100 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    140 us
  link        bench_rcc_o1:  45498 us
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
| TCC       |       106 ms |
| GCC -O0   |       961 ms |
| GCC -O2   |      9244 ms |
| Clang -O0 |       937 ms |
| Clang -O2 |      9003 ms |
