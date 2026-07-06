# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           61 |          657 |        718 |
| RCC -O1   |           50 |          636 |        686 |
| TCC       |           47 |          594 |        641 |
| GCC -O0   |          128 |          444 |        572 |
| GCC -O2   |          173 |          273 |        446 |
| Clang -O0 |           63 |          517 |        580 |
| Clang -O2 |          108 |          289 |        397 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    562 us
  lex         bench.c:     77 us
  parse       bench.c:    109 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    151 us
  link        bench_rcc:  69957 us

RCC -O1:
  preprocess  bench.c:    451 us
  lex         bench.c:     73 us
  parse       bench.c:    100 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    138 us
  link        bench_rcc_o1:  63060 us
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
| GCC -O0   |      1174 ms |
| GCC -O2   |     12048 ms |
| Clang -O0 |      1341 ms |
| Clang -O2 |     15411 ms |
