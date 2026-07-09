# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           80 |          731 |        811 |
| RCC -O1   |           93 |          780 |        873 |
| TCC       |           77 |          612 |        689 |
| GCC -O0   |          127 |          508 |        635 |
| GCC -O2   |          164 |          299 |        463 |
| Clang -O0 |           84 |          495 |        579 |
| Clang -O2 |          160 |          316 |        476 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    581 us
  lex         bench.c:     85 us
  parse       bench.c:    127 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    154 us
  link        bench_rcc:  69221 us

RCC -O1:
  preprocess  bench.c:    580 us
  lex         bench.c:     79 us
  parse       bench.c:    141 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     19 us
  codegen     bench.c:    147 us
  link        bench_rcc_o1:  59234 us
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
| TCC       |       147 ms |
| GCC -O0   |      2439 ms |
| GCC -O2   |     19129 ms |
| Clang -O0 |      1916 ms |
| Clang -O2 |     17836 ms |
