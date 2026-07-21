# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           63 |          768 |        831 |
| RCC -O1   |          192 |          728 |        920 |
| RCC -O2   |           98 |          738 |        836 |
| TCC       |           70 |          689 |        759 |
| GCC -O0   |          163 |          536 |        699 |
| GCC -O2   |          136 |          324 |        460 |
| Clang -O0 |           96 |          527 |        623 |
| Clang -O2 |          111 |          300 |        411 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2435 us
  parse       bench.c:    287 us
  typecheck   bench.c:     10 us
  codegen     bench.c:    272 us
  link        bench_rcc:  90120 us

RCC -O1:
  preprocess  bench.c:    715 us
  parse       bench.c:    134 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    133 us
  link        bench_rcc_o1:  69372 us

RCC -O2:
  preprocess  bench.c:    594 us
  parse       bench.c:    109 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    105 us
  link        bench_rcc_o2:  60581 us
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
| TCC       |       105 ms |
| GCC -O0   |      1384 ms |
| GCC -O2   |     16624 ms |
| Clang -O0 |      2032 ms |
| Clang -O2 |     19618 ms |
