# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           85 |          777 |        862 |
| RCC -O1   |           73 |          746 |        819 |
| RCC -O2   |           83 |          712 |        795 |
| TCC       |           85 |          731 |        816 |
| GCC -O0   |          158 |          629 |        787 |
| GCC -O2   |          213 |          366 |        579 |
| Clang -O0 |          142 |          646 |        788 |
| Clang -O2 |          178 |          396 |        574 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    784 us
  parse       bench.c:    145 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    139 us
  link        bench_rcc:  69420 us

RCC -O1:
  preprocess  bench.c:    807 us
  parse       bench.c:    146 us
  typecheck   bench.c:      4 us
  opt         bench.c:     14 us
  codegen     bench.c:    142 us
  link        bench_rcc_o1:  74602 us

RCC -O2:
  preprocess  bench.c:    739 us
  parse       bench.c:    127 us
  typecheck   bench.c:      4 us
  opt         bench.c:     15 us
  codegen     bench.c:    125 us
  link        bench_rcc_o2:  77266 us
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
| TCC       |       208 ms |
| GCC -O0   |      1841 ms |
| GCC -O2   |     15729 ms |
| Clang -O0 |      1698 ms |
| Clang -O2 |     16444 ms |
