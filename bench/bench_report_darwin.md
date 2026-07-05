# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           51 |          574 |        625 |
| RCC -O1   |           53 |          626 |        679 |
| TCC       |           37 |          512 |        549 |
| GCC -O0   |           65 |          449 |        514 |
| GCC -O2   |          110 |          266 |        376 |
| Clang -O0 |           68 |          436 |        504 |
| Clang -O2 |           95 |          267 |        362 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    477 us
  lex         bench.c:     74 us
  parse       bench.c:     96 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    137 us
  link        bench_rcc:  47414 us

RCC -O1:
  preprocess  bench.c:    420 us
  lex         bench.c:     68 us
  parse       bench.c:     86 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    133 us
  link        bench_rcc_o1:  48011 us
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
| TCC       |        97 ms |
| GCC -O0   |      1243 ms |
| GCC -O2   |     10272 ms |
| Clang -O0 |      1316 ms |
| Clang -O2 |     13429 ms |
