# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           59 |          677 |        736 |
| RCC -O1   |           56 |          624 |        680 |
| TCC       |           38 |          566 |        604 |
| GCC -O0   |          106 |          469 |        575 |
| GCC -O2   |          100 |          287 |        387 |
| Clang -O0 |           59 |          472 |        531 |
| Clang -O2 |           89 |          282 |        371 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    496 us
  lex         bench.c:     73 us
  parse       bench.c:     99 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    135 us
  link        bench_rcc:  47669 us

RCC -O1:
  preprocess  bench.c:    459 us
  lex         bench.c:     73 us
  parse       bench.c:     98 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    131 us
  link        bench_rcc_o1:  47757 us
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
| TCC       |       124 ms |
| GCC -O0   |      1169 ms |
| GCC -O2   |     11953 ms |
| Clang -O0 |      1141 ms |
| Clang -O2 |     10151 ms |
