# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           88 |          623 |        711 |
| RCC -O1   |           66 |          623 |        689 |
| TCC       |           34 |          512 |        546 |
| GCC -O0   |           61 |          433 |        494 |
| GCC -O2   |           90 |          262 |        352 |
| Clang -O0 |           51 |          434 |        485 |
| Clang -O2 |           74 |          262 |        336 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1134 us
  lex         bench.c:     77 us
  parse       bench.c:     95 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1727 us
  peephole    bench.c:    291 us (est, 778 calls)
  link        bench_rcc:  84736 us

RCC -O1:
  preprocess  bench.c:   3971 us
  lex         bench.c:     80 us
  parse       bench.c:     96 us
  typecheck   bench.c:      5 us
  opt(CTFE)   bench.c:     13 us
  codegen     bench.c:   2007 us
  peephole    bench.c:    290 us (est, 775 calls)
  link        bench_rcc_o1:  71217 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 749278 us
  lex         sqlite3.c:  74122 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 591996 us
  lex         sqlite3.c:  78253 us
sqlite3.c:1: [0m[1;31merror:[0m expected specific operator
 __sync_synchronize();
 [1;31m^~~~~~~~~~~~~~~~~~[0m
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        69 ms |
| GCC -O0   |       808 ms |
| GCC -O2   |      8382 ms |
| Clang -O0 |       799 ms |
| Clang -O2 |      8292 ms |
