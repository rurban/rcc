# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          664 |        717 |
| RCC -O1   |           54 |          605 |        659 |
| TCC       |           38 |          532 |        570 |
| GCC -O0   |           65 |          447 |        512 |
| GCC -O2   |          102 |          274 |        376 |
| Clang -O0 |           55 |          448 |        503 |
| Clang -O2 |           84 |          284 |        368 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    567 us
  lex         bench.c:     75 us
  parse       bench.c:    106 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    150 us
  link        bench_rcc:  56917 us

RCC -O1:
  preprocess  bench.c:    627 us
  lex         bench.c:     76 us
  parse       bench.c:    109 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    139 us
  link        bench_rcc_o1:  53656 us
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
| TCC       |        91 ms |
| GCC -O0   |       936 ms |
| GCC -O2   |      9541 ms |
| Clang -O0 |       995 ms |
| Clang -O2 |      9421 ms |
