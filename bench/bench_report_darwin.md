# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           92 |          717 |        809 |
| RCC -O1   |          104 |          903 |       1007 |
| RCC -O2   |          117 |          812 |        929 |
| TCC       |           73 |          683 |        756 |
| GCC -O0   |          115 |          501 |        616 |
| GCC -O2   |          160 |          303 |        463 |
| Clang -O0 |           97 |          556 |        653 |
| Clang -O2 |          154 |          304 |        458 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    959 us
  parse       bench.c:    135 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    143 us
  link        bench_rcc:  75860 us

RCC -O1:
  preprocess  bench.c:    743 us
  parse       bench.c:    136 us
  typecheck   bench.c:      6 us
  opt         bench.c:     21 us
  codegen     bench.c:    142 us
  link        bench_rcc_o1:  76844 us

RCC -O2:
  preprocess  bench.c:    734 us
  parse       bench.c:    158 us
  typecheck   bench.c:      6 us
  opt         bench.c:     27 us
  codegen     bench.c:    127 us
  link        bench_rcc_o2:  65637 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 373044 us
  parse       sqlite3.c:  87547 us
  typecheck   sqlite3.c:  28807 us
  codegen     sqlite3.c:  60329 us

RCC -O1:
  preprocess  sqlite3.c: 291736 us
  parse       sqlite3.c:  60797 us
  typecheck   sqlite3.c:  15790 us
  opt         sqlite3.c:  23843 us
  codegen     sqlite3.c:  68785 us

RCC -O2:
  preprocess  sqlite3.c: 333749 us
  parse       sqlite3.c:  77078 us
  typecheck   sqlite3.c:  31627 us
  opt         sqlite3.c: 179983 us
  codegen     sqlite3.c:  54855 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1068 ms |
| RCC -O1   |       870 ms |
| RCC -O2   |      1015 ms |
| TCC       |       197 ms |
| GCC -O0   |      1466 ms |
| GCC -O2   |     12146 ms |
| Clang -O0 |      1116 ms |
| Clang -O2 |     13058 ms |
