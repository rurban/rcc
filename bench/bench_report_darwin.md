# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           82 |          751 |        833 |
| RCC -O1   |           95 |          707 |        802 |
| RCC -O2   |           73 |          669 |        742 |
| TCC       |           54 |          616 |        670 |
| GCC -O0   |          124 |          518 |        642 |
| GCC -O2   |          149 |          327 |        476 |
| Clang -O0 |           85 |          524 |        609 |
| Clang -O2 |          184 |          352 |        536 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    721 us
  parse       bench.c:    120 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    137 us
  link        bench_rcc:  68988 us

RCC -O1:
  preprocess  bench.c:    615 us
  parse       bench.c:    138 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    127 us
  link        bench_rcc_o1:  64183 us

RCC -O2:
  preprocess  bench.c:    618 us
  parse       bench.c:    125 us
  typecheck   bench.c:      6 us
  opt         bench.c:     20 us
  codegen     bench.c:    119 us
  link        bench_rcc_o2:  67221 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       291 ms |
| GCC -O0   |      1565 ms |
| GCC -O2   |     16332 ms |
| Clang -O0 |      2393 ms |
| Clang -O2 |     15906 ms |
