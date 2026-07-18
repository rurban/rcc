# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           52 |          572 |        624 |
| RCC -O1   |           54 |          624 |        678 |
| TCC       |           40 |          514 |        554 |
| GCC -O0   |           59 |          434 |        493 |
| GCC -O2   |          103 |          263 |        366 |
| Clang -O0 |           53 |          433 |        486 |
| Clang -O2 |          104 |          275 |        379 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    591 us
  parse       bench.c:     93 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    110 us
  link        bench_rcc:  45185 us

RCC -O1:
  preprocess  bench.c:    494 us
  parse       bench.c:     93 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     15 us
  codegen     bench.c:    104 us
  link        bench_rcc_o1:  44113 us
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
| TCC       |        97 ms |
| GCC -O0   |      1054 ms |
| GCC -O2   |      9425 ms |
| Clang -O0 |       986 ms |
| Clang -O2 |     10444 ms |
