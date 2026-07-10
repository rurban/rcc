# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           78 |          701 |        779 |
| RCC -O1   |           76 |          639 |        715 |
| TCC       |           74 |          557 |        631 |
| GCC -O0   |          136 |          472 |        608 |
| GCC -O2   |          134 |          286 |        420 |
| Clang -O0 |           78 |          475 |        553 |
| Clang -O2 |          119 |          286 |        405 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1518 us
  lex         bench.c:    107 us
  parse       bench.c:    145 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    189 us
  link        bench_rcc: 114164 us

RCC -O1:
  preprocess  bench.c:    587 us
  lex         bench.c:     82 us
  parse       bench.c:    147 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    160 us
  link        bench_rcc_o1:  66002 us
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
| TCC       |       118 ms |
| GCC -O0   |      1104 ms |
| GCC -O2   |     12038 ms |
| Clang -O0 |      1564 ms |
| Clang -O2 |     12929 ms |
