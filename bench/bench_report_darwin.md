# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          573 |        626 |
| RCC -O1   |           53 |          626 |        679 |
| TCC       |           35 |          512 |        547 |
| GCC -O0   |           62 |          434 |        496 |
| GCC -O2   |          103 |          265 |        368 |
| Clang -O0 |           53 |          435 |        488 |
| Clang -O2 |           91 |          263 |        354 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    524 us
  lex         bench.c:     77 us
  parse       bench.c:    126 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    142 us
  link        bench_rcc:  46840 us

RCC -O1:
  preprocess  bench.c:    513 us
  lex         bench.c:     71 us
  parse       bench.c:    162 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    133 us
  link        bench_rcc_o1:  50083 us
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
| TCC       |        88 ms |
| GCC -O0   |       924 ms |
| GCC -O2   |      8969 ms |
| Clang -O0 |       941 ms |
| Clang -O2 |      9086 ms |
