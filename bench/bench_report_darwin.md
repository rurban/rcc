# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           53 |          574 |        627 |
| RCC -O1   |           53 |          625 |        678 |
| TCC       |           34 |          512 |        546 |
| GCC -O0   |           61 |          435 |        496 |
| GCC -O2   |           93 |          263 |        356 |
| Clang -O0 |           53 |          434 |        487 |
| Clang -O2 |           89 |          263 |        352 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    559 us
  lex         bench.c:     82 us
  parse       bench.c:    130 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    166 us
  link        bench_rcc:  48815 us

RCC -O1:
  preprocess  bench.c:    558 us
  lex         bench.c:     76 us
  parse       bench.c:    107 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     17 us
  codegen     bench.c:    145 us
  link        bench_rcc_o1:  47919 us
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
| TCC       |        89 ms |
| GCC -O0   |       940 ms |
| GCC -O2   |      8773 ms |
| Clang -O0 |       924 ms |
| Clang -O2 |      8627 ms |
