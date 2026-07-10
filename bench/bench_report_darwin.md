# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           69 |          723 |        792 |
| RCC -O1   |           99 |          660 |        759 |
| TCC       |           58 |          667 |        725 |
| GCC -O0   |          175 |          497 |        672 |
| GCC -O2   |          172 |          298 |        470 |
| Clang -O0 |          121 |          501 |        622 |
| Clang -O2 |          129 |          285 |        414 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    782 us
  lex         bench.c:     76 us
  parse       bench.c:    105 us
  typecheck   bench.c:      3 us
  codegen     bench.c:    160 us
  link        bench_rcc:  59469 us

RCC -O1:
  preprocess  bench.c:    498 us
  lex         bench.c:     72 us
  parse       bench.c:     99 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:    142 us
  link        bench_rcc_o1:  73526 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
sysctl.h:794: error: #error Use the SYSCTL_*() macros and (-1) instead!

RCC -O1:
sysctl.h:794: error: #error Use the SYSCTL_*() macros and (-1) instead!
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       157 ms |
| GCC -O0   |      1293 ms |
| GCC -O2   |     12107 ms |
| Clang -O0 |      1238 ms |
| Clang -O2 |     13028 ms |
