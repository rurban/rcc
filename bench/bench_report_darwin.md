# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          677 |        764 |
| RCC -O1   |           96 |          733 |        829 |
| TCC       |           55 |          575 |        630 |
| GCC -O0   |          120 |          479 |        599 |
| GCC -O2   |          167 |          288 |        455 |
| Clang -O0 |           68 |          483 |        551 |
| Clang -O2 |          111 |          289 |        400 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    790 us
  lex         bench.c:     83 us
  parse       bench.c:    118 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    144 us
  link        bench_rcc:  56479 us

RCC -O1:
  preprocess  bench.c:    515 us
  lex         bench.c:     86 us
  parse       bench.c:    113 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:    160 us
  link        bench_rcc_o1:  64907 us
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
| TCC       |       292 ms |
| GCC -O0   |      2041 ms |
| GCC -O2   |     15705 ms |
| Clang -O0 |      1247 ms |
| Clang -O2 |     14458 ms |
