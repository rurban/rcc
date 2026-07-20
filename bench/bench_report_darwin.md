# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           76 |          768 |        844 |
| RCC -O1   |           83 |          858 |        941 |
| RCC -O2   |          135 |          683 |        818 |
| TCC       |          113 |          591 |        704 |
| GCC -O0   |          158 |          606 |        764 |
| GCC -O2   |          185 |          333 |        518 |
| Clang -O0 |           81 |          542 |        623 |
| Clang -O2 |          163 |          329 |        492 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1363 us
  parse       bench.c:    150 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    190 us
  link        bench_rcc: 160273 us

RCC -O1:
  preprocess  bench.c:    649 us
  parse       bench.c:    137 us
  typecheck   bench.c:      5 us
  opt         bench.c:     16 us
  codegen     bench.c:    141 us
  link        bench_rcc_o1: 259472 us

RCC -O2:
  preprocess  bench.c:   1638 us
  parse       bench.c:    113 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    158 us
  link        bench_rcc_o2: 188135 us
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
| TCC       |       161 ms |
| GCC -O0   |      1239 ms |
| GCC -O2   |     14468 ms |
| Clang -O0 |      1799 ms |
| Clang -O2 |     13918 ms |
