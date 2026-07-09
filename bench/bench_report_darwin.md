# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           99 |          692 |        791 |
| RCC -O1   |          100 |          675 |        775 |
| TCC       |           46 |          556 |        602 |
| GCC -O0   |           94 |          519 |        613 |
| GCC -O2   |          164 |          335 |        499 |
| Clang -O0 |          120 |          558 |        678 |
| Clang -O2 |          118 |          332 |        450 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    858 us
  lex         bench.c:     74 us
  parse       bench.c:    137 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    142 us
  link        bench_rcc:  66792 us

RCC -O1:
  preprocess  bench.c:    542 us
  lex         bench.c:     72 us
  parse       bench.c:     96 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    137 us
  link        bench_rcc_o1:  65896 us
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
| TCC       |       174 ms |
| GCC -O0   |      2542 ms |
| GCC -O2   |     22255 ms |
| Clang -O0 |      2095 ms |
| Clang -O2 |     17302 ms |
