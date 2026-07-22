# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           87 |          674 |        761 |
| RCC -O1   |           83 |          623 |        706 |
| RCC -O2   |           87 |          665 |        752 |
| TCC       |          147 |          655 |        802 |
| GCC -O0   |          112 |          548 |        660 |
| GCC -O2   |          207 |          300 |        507 |
| Clang -O0 |           86 |          520 |        606 |
| Clang -O2 |          163 |          313 |        476 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    785 us
  parse       bench.c:    136 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    124 us
  link        bench_rcc:  65913 us

RCC -O1:
  preprocess  bench.c:    593 us
  parse       bench.c:    119 us
  typecheck   bench.c:      4 us
  opt         bench.c:     14 us
  codegen     bench.c:    116 us
  link        bench_rcc_o1:  64005 us

RCC -O2:
  preprocess  bench.c:    655 us
  parse       bench.c:    133 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    122 us
  link        bench_rcc_o2:  67664 us
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
| TCC       |       129 ms |
| GCC -O0   |      1103 ms |
| GCC -O2   |     12303 ms |
| Clang -O0 |      1408 ms |
| Clang -O2 |     11001 ms |
