# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          608 |        662 |
| RCC -O1   |           49 |          667 |        716 |
| TCC       |           49 |          544 |        593 |
| GCC -O0   |           66 |          483 |        549 |
| GCC -O2   |          115 |          284 |        399 |
| Clang -O0 |           54 |          458 |        512 |
| Clang -O2 |           92 |          277 |        369 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1128 us
  lex         bench.c:     64 us
  parse       bench.c:    111 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    111 us
  link        bench_rcc:  56460 us

RCC -O1:
  preprocess  bench.c:    455 us
  lex         bench.c:     60 us
  parse       bench.c:    103 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    103 us
  link        bench_rcc_o1:  47511 us
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
| TCC       |       100 ms |
| GCC -O0   |       979 ms |
| GCC -O2   |      9954 ms |
| Clang -O0 |       999 ms |
| Clang -O2 |      9317 ms |
