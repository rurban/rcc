# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          133 |          872 |       1005 |
| RCC -O1   |          137 |          749 |        886 |
| TCC       |           83 |          579 |        662 |
| GCC -O0   |          132 |          455 |        587 |
| GCC -O2   |          139 |          274 |        413 |
| Clang -O0 |           60 |          451 |        511 |
| Clang -O2 |          222 |          295 |        517 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    631 us
  lex         bench.c:     82 us
  parse       bench.c:    152 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    185 us
  link        bench_rcc: 108502 us

RCC -O1:
  preprocess  bench.c:    511 us
  lex         bench.c:     87 us
  parse       bench.c:    129 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    168 us
  link        bench_rcc_o1:  84626 us
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
| TCC       |       150 ms |
| GCC -O0   |      1367 ms |
| GCC -O2   |     13968 ms |
| Clang -O0 |      1370 ms |
| Clang -O2 |     10854 ms |
