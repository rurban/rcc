# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          674 |        771 |
| RCC -O1   |           71 |          672 |        743 |
| TCC       |           37 |          551 |        588 |
| GCC -O0   |           66 |          463 |        529 |
| GCC -O2   |           92 |          283 |        375 |
| Clang -O0 |           55 |          465 |        520 |
| Clang -O2 |           80 |          281 |        361 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2720 us
  lex         bench.c:     84 us
  parse       bench.c:    125 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1748 us
  peephole    bench.c:    291 us (est, 778 calls)
  link        bench_rcc:  70011 us

RCC -O1:
  preprocess  bench.c:   1633 us
  lex         bench.c:     80 us
  parse       bench.c:     79 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   1366 us
  peephole    bench.c:    290 us (est, 775 calls)
  link        bench_rcc_o1:  68933 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 845387 us
  lex         sqlite3.c:  76916 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 710521 us
  lex         sqlite3.c:  82836 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        80 ms |
| GCC -O0   |      1083 ms |
| GCC -O2   |      9615 ms |
| Clang -O0 |       934 ms |
| Clang -O2 |      9553 ms |
