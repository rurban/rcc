# Darwin RCC Benchmark Results

_Generated: June 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           95 |          596 |        691 |
| RCC -O1   |           73 |          606 |        679 |
| TCC       |           37 |          517 |        554 |
| GCC -O0   |           65 |          457 |        522 |
| GCC -O2   |          103 |          277 |        380 |
| Clang -O0 |           62 |          463 |        525 |
| Clang -O2 |           89 |          283 |        372 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1001 us
  lex         bench.c:    101 us
  parse       bench.c:     61 us
  typecheck   bench.c:      4 us
  codegen     bench.c:   1468 us
  peephole    bench.c:    242 us
  link        bench_rcc:  65953 us

RCC -O1:
  preprocess  bench.c:    611 us
  lex         bench.c:     99 us
  parse       bench.c:     59 us
  typecheck   bench.c:      4 us
  opt(CTFE)   bench.c:     11 us
  codegen     bench.c:   1994 us
  peephole    bench.c:    232 us
  link        bench_rcc_o1:  64526 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 333692 us
  lex         sqlite3.c: 126482 us
sqlite3.c:1: [0m[1;31merror:[0m expected ';', ',', or '{'
 __DARWIN_ALIAS_STARTING_MAC_1060(__asm("_" "setmode" ));
 [1;31m^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[0m

RCC -O1:
  preprocess  sqlite3.c: 325600 us
  lex         sqlite3.c: 122451 us
sqlite3.c:1: [0m[1;31merror:[0m expected ';', ',', or '{'
 __DARWIN_ALIAS_STARTING_MAC_1060(__asm("_" "setmode" ));
 [1;31m^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~[0m
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |        452ms |
| RCC -O1   |        413ms |
| TCC       |         81ms |
| GCC -O0   |        967ms |
| GCC -O2   |       9596ms |
| Clang -O0 |        989ms |
| Clang -O2 |       9320ms |
