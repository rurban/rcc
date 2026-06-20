# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           83 |          616 |        699 |
| RCC -O1   |           87 |          682 |        769 |
| TCC       |           43 |          569 |        612 |
| GCC -O0   |           79 |          481 |        560 |
| GCC -O2   |          119 |          284 |        403 |
| Clang -O0 |           59 |          468 |        527 |
| Clang -O2 |          101 |          282 |        383 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    780 us
  lex         bench.c:     74 us
  parse       bench.c:     84 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   1315 us
  peephole    bench.c:    233 us
  link        bench_rcc:  61516 us

RCC -O1:
  preprocess  bench.c:    723 us
  lex         bench.c:     74 us
  parse       bench.c:     75 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     14 us
  codegen     bench.c:   1365 us
  peephole    bench.c:    226 us
  link        bench_rcc_o1:  61839 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 414688 us
  lex         sqlite3.c: 100820 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 304145 us
  lex         sqlite3.c:  63128 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        81 ms |
| GCC -O0   |       951 ms |
| GCC -O2   |      9696 ms |
| Clang -O0 |       909 ms |
| Clang -O2 |      9473 ms |
