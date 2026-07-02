# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           91 |          679 |        770 |
| RCC -O1   |           93 |          771 |        864 |
| TCC       |           55 |          602 |        657 |
| GCC -O0   |          130 |          523 |        653 |
| GCC -O2   |          151 |          332 |        483 |
| Clang -O0 |          122 |          543 |        665 |
| Clang -O2 |          191 |          341 |        532 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    947 us
  lex         bench.c:     71 us
  parse       bench.c:     71 us
  typecheck   bench.c:      4 us
  codegen     bench.c:     97 us
  peephole    bench.c:     47 us
  link        bench_rcc:  41121 us

RCC -O1:
  preprocess  bench.c:   1093 us
  lex         bench.c:     71 us
  parse       bench.c:     72 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     12 us
  codegen     bench.c:     74 us
  peephole    bench.c:     47 us
  link        bench_rcc_o1:  40581 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 916735 us
  lex         sqlite3.c:  89367 us
  parse       sqlite3.c:  58221 us
  typecheck   sqlite3.c:  18605 us
  codegen     sqlite3.c: 408959 us
  peephole    sqlite3.c: 2234399 us

RCC -O1:
  preprocess  sqlite3.c: 924910 us
  lex         sqlite3.c:  88130 us
  parse       sqlite3.c: 118268 us
  typecheck   sqlite3.c:  23942 us
  opt(CTFE)   sqlite3.c:  37234 us
  codegen     sqlite3.c: 459260 us
  peephole    sqlite3.c: 2394908 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      5924 ms |
| RCC -O1   |      4620 ms |
| TCC       |        92 ms |
| GCC -O0   |       940 ms |
| GCC -O2   |     14378 ms |
| Clang -O0 |      1692 ms |
| Clang -O2 |     18988 ms |
