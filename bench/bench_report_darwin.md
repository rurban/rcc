# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          140 |          648 |        788 |
| RCC -O1   |          125 |          704 |        829 |
| TCC       |           60 |          573 |        633 |
| GCC -O0   |           92 |          497 |        589 |
| GCC -O2   |          132 |          291 |        423 |
| Clang -O0 |          101 |          478 |        579 |
| Clang -O2 |          125 |          313 |        438 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2267 us
  lex         bench.c:     91 us
  parse       bench.c:     96 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2547 us
  peephole    bench.c:    292 us (est, 779 calls, pat3=0)
  link        bench_rcc:  78421 us

RCC -O1:
  preprocess  bench.c:   2295 us
  lex         bench.c:     86 us
  parse       bench.c:     89 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   1739 us
  peephole    bench.c:    291 us (est, 776 calls, pat3=0)
  link        bench_rcc_o1:  81310 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1408739 us
  lex         sqlite3.c: 102653 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 931487 us
  lex         sqlite3.c: 102996 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       132 ms |
| GCC -O0   |      1357 ms |
| GCC -O2   |     12295 ms |
| Clang -O0 |      1144 ms |
| Clang -O2 |     11512 ms |
