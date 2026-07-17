# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |         1015 |          798 |       1813 |
| RCC -O1   |          276 |          954 |       1230 |
| TCC       |          103 |          758 |        861 |
| GCC -O0   |          193 |          602 |        795 |
| GCC -O2   |          144 |          347 |        491 |
| Clang -O0 |          221 |          632 |        853 |
| Clang -O2 |          207 |          378 |        585 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    796 us
  lex         bench.c:     66 us
  parse       bench.c:    230 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    212 us
  link        bench_rcc: 121711 us

RCC -O1:
  preprocess  bench.c:   1443 us
  lex         bench.c:    185 us
  parse       bench.c:    299 us
  typecheck   bench.c:     10 us
  opt(CTFE)   bench.c:     45 us
  codegen     bench.c:    315 us
  link        bench_rcc_o1: 131310 us
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
| TCC       |       256 ms |
| GCC -O0   |      1602 ms |
| GCC -O2   |     13014 ms |
| Clang -O0 |      1380 ms |
| Clang -O2 |     16057 ms |
