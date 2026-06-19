# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          193 |          686 |        879 |
| RCC -O1   |           94 |          702 |        796 |
| TCC       |           84 |          714 |        798 |
| GCC -O0   |          179 |          581 |        760 |
| GCC -O2   |          226 |          363 |        589 |
| Clang -O0 |           88 |          606 |        694 |
| Clang -O2 |          180 |          328 |        508 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   2000 us
  lex         bench.c:     89 us
  parse       bench.c:     97 us
  typecheck   bench.c:      5 us
  codegen     bench.c:   2503 us
  peephole    bench.c:    292 us (est, 779 calls, pat3=0)
  link        bench_rcc: 107872 us

RCC -O1:
  preprocess  bench.c:   2503 us
  lex         bench.c:     90 us
  parse       bench.c:     87 us
  typecheck   bench.c:     38 us
  opt(CTFE)   bench.c:    135 us
  codegen     bench.c:   3416 us
  peephole    bench.c:    291 us (est, 776 calls, pat3=0)
  link        bench_rcc_o1: 105672 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 1423683 us
  lex         sqlite3.c: 133110 us
  parse       sqlite3.c: 113573 us
  typecheck   sqlite3.c:  25550 us
  codegen     sqlite3.c: 1293463 us
  peephole    sqlite3.c: 203976 us (est, 543938 calls, pat3=66)
  link        null: 6293003 us

RCC -O1:
  preprocess  sqlite3.c: 977655 us
  lex         sqlite3.c: 100852 us
  parse       sqlite3.c: 105598 us
  typecheck   sqlite3.c:  22694 us
  opt(CTFE)   sqlite3.c:  23832 us
  codegen     sqlite3.c: 1257145 us
  peephole    sqlite3.c: 202746 us (est, 540657 calls, pat3=66)
  link        null: 6274659 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |     10525 ms |
| RCC -O1   |      9422 ms |
| TCC       |       161 ms |
| GCC -O0   |      1375 ms |
| GCC -O2   |     13575 ms |
| Clang -O0 |       998 ms |
| Clang -O2 |     14183 ms |
