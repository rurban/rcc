# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           76 |          659 |        735 |
| RCC -O1   |           63 |          623 |        686 |
| RCC -O2   |           61 |          637 |        698 |
| TCC       |           43 |          541 |        584 |
| GCC -O0   |          100 |          446 |        546 |
| GCC -O2   |           94 |          264 |        358 |
| Clang -O0 |           80 |          451 |        531 |
| Clang -O2 |          113 |          273 |        386 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    755 us
  parse       bench.c:    123 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    133 us
  link        bench_rcc:  63409 us

RCC -O1:
  preprocess  bench.c:    839 us
  parse       bench.c:    184 us
  typecheck   bench.c:      6 us
  opt         bench.c:     19 us
  codegen     bench.c:    160 us
  link        bench_rcc_o1:  64751 us

RCC -O2:
  preprocess  bench.c:    742 us
  parse       bench.c:    230 us
  typecheck   bench.c:      5 us
  opt         bench.c:     27 us
  codegen     bench.c:    136 us
  link        bench_rcc_o2:  54778 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 320656 us
  parse       sqlite3.c:  51994 us
  typecheck   sqlite3.c:  11403 us
  codegen     sqlite3.c:  49459 us

RCC -O1:
  preprocess  sqlite3.c: 222242 us
  parse       sqlite3.c:  41028 us
  typecheck   sqlite3.c:  11720 us
  opt         sqlite3.c:  21797 us
  codegen     sqlite3.c:  49364 us

RCC -O2:
  preprocess  sqlite3.c: 220971 us
  parse       sqlite3.c:  55091 us
  typecheck   sqlite3.c:  12287 us
  opt         sqlite3.c: 142789 us
  codegen     sqlite3.c:  58594 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       395 ms |
| RCC -O1   |       418 ms |
| RCC -O2   |       461 ms |
| TCC       |        65 ms |
| GCC -O0   |      1136 ms |
| GCC -O2   |     10734 ms |
| Clang -O0 |      1260 ms |
| Clang -O2 |      9494 ms |
