# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           50 |          620 |        670 |
| RCC -O1   |           51 |          581 |        632 |
| RCC -O2   |           60 |          582 |        642 |
| TCC       |           41 |          513 |        554 |
| GCC -O0   |           64 |          433 |        497 |
| GCC -O2   |           99 |          263 |        362 |
| Clang -O0 |           57 |          433 |        490 |
| Clang -O2 |           84 |          263 |        347 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    760 us
  parse       bench.c:    121 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    156 us
  link        bench_rcc:  55072 us

RCC -O1:
  preprocess  bench.c:    574 us
  parse       bench.c:    131 us
  typecheck   bench.c:      4 us
  opt         bench.c:     20 us
  codegen     bench.c:    116 us
  link        bench_rcc_o1:  52441 us
```

RCC -O2:
preprocess bench.c: 609 us
parse bench.c: 116 us
typecheck bench.c: 5 us
opt bench.c: 18 us
codegen bench.c: 116 us
link bench_rcc_o2: 53100 us

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
| TCC       |        90 ms |
| GCC -O0   |       979 ms |
| GCC -O2   |      8563 ms |
| Clang -O0 |       976 ms |
| Clang -O2 |      9126 ms |
