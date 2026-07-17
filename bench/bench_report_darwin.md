# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           54 |          621 |        675 |
| RCC -O1   |           60 |          684 |        744 |
| TCC       |           51 |          562 |        613 |
| GCC -O0   |           91 |          473 |        564 |
| GCC -O2   |          114 |          282 |        396 |
| Clang -O0 |           61 |          473 |        534 |
| Clang -O2 |           87 |          285 |        372 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    595 us
  lex         bench.c:     79 us
  parse       bench.c:    109 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    114 us
  link        bench_rcc:  50434 us

RCC -O1:
  preprocess  bench.c:    474 us
  lex         bench.c:     82 us
  parse       bench.c:    104 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    111 us
  link        bench_rcc_o1:  50240 us
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
| TCC       |       101 ms |
| GCC -O0   |      1056 ms |
| GCC -O2   |     10577 ms |
| Clang -O0 |      1009 ms |
| Clang -O2 |     10064 ms |
