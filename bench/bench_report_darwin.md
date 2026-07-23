# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |          108 |          838 |        946 |
| RCC -O1   |           93 |          792 |        885 |
| RCC -O2   |          108 |          797 |        905 |
| TCC       |           98 |          751 |        849 |
| GCC -O0   |          219 |          642 |        861 |
| GCC -O2   |          168 |          466 |        634 |
| Clang -O0 |          264 |          644 |        908 |
| Clang -O2 |          150 |          349 |        499 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:   1576 us
  parse       bench.c:    179 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    170 us
  link        bench_rcc:  83013 us

RCC -O1:
  preprocess  bench.c:    638 us
  parse       bench.c:    133 us
  typecheck   bench.c:      5 us
  opt         bench.c:     20 us
  codegen     bench.c:    140 us
  link        bench_rcc_o1:  62760 us

RCC -O2:
  preprocess  bench.c:    667 us
  parse       bench.c:    149 us
  typecheck   bench.c:      5 us
  opt         bench.c:     25 us
  codegen     bench.c:    150 us
  link        bench_rcc_o2:  79979 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 490851 us
  parse       sqlite3.c: 173316 us
  typecheck   sqlite3.c:  21869 us
  codegen     sqlite3.c:  91201 us

RCC -O1:
  preprocess  sqlite3.c: 370765 us
  parse       sqlite3.c:  71024 us
  typecheck   sqlite3.c:  23984 us
  opt         sqlite3.c:  71650 us
  codegen     sqlite3.c: 131248 us

RCC -O2:
  preprocess  sqlite3.c: 354756 us
  parse       sqlite3.c:  64409 us
  typecheck   sqlite3.c:  38058 us
  opt         sqlite3.c: 211133 us
  codegen     sqlite3.c:  78936 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1046 ms |
| RCC -O1   |       693 ms |
| RCC -O2   |       798 ms |
| TCC       |       131 ms |
| GCC -O0   |      1694 ms |
| GCC -O2   |     17780 ms |
| Clang -O0 |      1734 ms |
| Clang -O2 |     16262 ms |
