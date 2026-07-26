# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          145 |          866 |       1011 |
| RCC -O1   |          181 |          825 |       1006 |
| RCC -O2   |          116 |          887 |       1003 |
| TCC       |          103 |          815 |        918 |
| GCC -O0   |          177 |          707 |        884 |
| GCC -O2   |          189 |          382 |        571 |
| Clang -O0 |          115 |          648 |        763 |
| Clang -O2 |          201 |          344 |        545 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1003 us
  parse       bench.c:    123 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    141 us
  link        bench_rcc:  69327 us

RCC -O1:
  preprocess  bench.c:    681 us
  parse       bench.c:    128 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    115 us
  link        bench_rcc_o1:  65302 us

RCC -O2:
  preprocess  bench.c:    667 us
  parse       bench.c:    353 us
  typecheck   bench.c:     11 us
  opt         bench.c:     51 us
  codegen     bench.c:    290 us
  link        bench_rcc_o2:  64308 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 434670 us
  parse       sqlite3.c: 158699 us
  typecheck   sqlite3.c:  19631 us
  codegen     sqlite3.c:  68475 us

RCC -O1:
  preprocess  sqlite3.c: 335354 us
  parse       sqlite3.c:  61092 us
  typecheck   sqlite3.c:  17681 us
  opt         sqlite3.c:  22148 us
  codegen     sqlite3.c:  64320 us

RCC -O2:
  preprocess  sqlite3.c: 351594 us
  parse       sqlite3.c:  61172 us
  typecheck   sqlite3.c:  17838 us
  opt         sqlite3.c: 281049 us
  codegen     sqlite3.c:  88545 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1494 ms |
| RCC -O1   |       878 ms |
| RCC -O2   |       868 ms |
| TCC       |        85 ms |
| GCC -O0   |      1041 ms |
| GCC -O2   |     13087 ms |
| Clang -O0 |      1356 ms |
| Clang -O2 |     13074 ms |
