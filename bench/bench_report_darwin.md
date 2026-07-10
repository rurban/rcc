# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           60 |          630 |        690 |
| RCC -O1   |           82 |          816 |        898 |
| TCC       |          240 |          621 |        861 |
| GCC -O0   |          115 |          535 |        650 |
| GCC -O2   |          236 |          337 |        573 |
| Clang -O0 |          116 |          510 |        626 |
| Clang -O2 |          140 |          315 |        455 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    624 us
  lex         bench.c:     79 us
  parse       bench.c:    129 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    194 us
  link        bench_rcc:  57211 us

RCC -O1:
  preprocess  bench.c:    541 us
  lex         bench.c:     74 us
  parse       bench.c:    104 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     17 us
  codegen     bench.c:    157 us
  link        bench_rcc_o1:  69762 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       168 ms |
| GCC -O0   |      1658 ms |
| GCC -O2   |     21334 ms |
| Clang -O0 |      2103 ms |
| Clang -O2 |     16832 ms |
