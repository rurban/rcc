# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          712 |        786 |
| RCC -O1   |           88 |          744 |        832 |
| RCC -O2   |          122 |          802 |        924 |
| TCC       |           88 |          733 |        821 |
| GCC -O0   |          258 |          661 |        919 |
| GCC -O2   |          273 |          363 |        636 |
| Clang -O0 |          127 |          566 |        693 |
| Clang -O2 |          291 |          344 |        635 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1020 us
  parse       bench.c:    130 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    123 us
  link        bench_rcc:  66443 us

RCC -O1:
  preprocess  bench.c:    560 us
  parse       bench.c:    129 us
  typecheck   bench.c:      5 us
  opt         bench.c:     15 us
  codegen     bench.c:    111 us
  link        bench_rcc_o1:  59371 us
```

RCC -O2:
preprocess bench.c: 572 us
parse bench.c: 96 us
typecheck bench.c: 4 us
opt bench.c: 14 us
codegen bench.c: 106 us
link bench_rcc_o2: 59623 us

```

## RCC Substep Timing -- sqlite3.c

```

RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

```

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       190 ms |
| GCC -O0   |      1927 ms |
| GCC -O2   |     15126 ms |
| Clang -O0 |      1541 ms |
| Clang -O2 |     14326 ms |
