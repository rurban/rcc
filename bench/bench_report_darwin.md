# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          125 |          782 |        907 |
| RCC -O1   |           61 |          653 |        714 |
| RCC -O2   |          146 |          709 |        855 |
| TCC       |         1464 |          556 |       2020 |
| GCC -O0   |          339 |          473 |        812 |
| GCC -O2   |          138 |          311 |        449 |
| Clang -O0 |          122 |          606 |        728 |
| Clang -O2 |          182 |          373 |        555 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    790 us
  parse       bench.c:    167 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    144 us
  link        bench_rcc:  81011 us

RCC -O1:
  preprocess  bench.c:    730 us
  parse       bench.c:    145 us
  typecheck   bench.c:      5 us
  opt         bench.c:     17 us
  codegen     bench.c:    129 us
  link        bench_rcc_o1:  77723 us

RCC -O2:
  preprocess  bench.c:    723 us
  parse       bench.c:    146 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    133 us
  link        bench_rcc_o2:  64516 us
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
| TCC       |       259 ms |
| GCC -O0   |      2438 ms |
| GCC -O2   |     19834 ms |
| Clang -O0 |      2187 ms |
| Clang -O2 |     21876 ms |
