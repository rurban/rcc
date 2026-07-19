# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           55 |          672 |        727 |
| RCC -O1   |           54 |          614 |        668 |
| RCC -O2   |           65 |          617 |        682 |
| TCC       |           45 |          524 |        569 |
| GCC -O0   |           81 |          435 |        516 |
| GCC -O2   |           96 |          270 |        366 |
| Clang -O0 |           62 |          454 |        516 |
| Clang -O2 |          102 |          264 |        366 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    627 us
  parse       bench.c:    123 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    107 us
  link        bench_rcc:  53264 us

RCC -O1:
  preprocess  bench.c:    569 us
  parse       bench.c:    105 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    107 us
  link        bench_rcc_o1:  52570 us
```

RCC -O2:
preprocess bench.c: 549 us
parse bench.c: 109 us
typecheck bench.c: 5 us
opt bench.c: 16 us
codegen bench.c: 109 us
link bench_rcc_o2: 56359 us

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
| TCC       |        99 ms |
| GCC -O0   |       938 ms |
| GCC -O2   |     10085 ms |
| Clang -O0 |       978 ms |
| Clang -O2 |     10026 ms |
