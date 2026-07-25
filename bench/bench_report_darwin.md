# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           97 |          869 |        966 |
| RCC -O1   |          145 |          794 |        939 |
| RCC -O2   |          100 |          821 |        921 |
| TCC       |           74 |          681 |        755 |
| GCC -O0   |          126 |          598 |        724 |
| GCC -O2   |          192 |          344 |        536 |
| Clang -O0 |           87 |          586 |        673 |
| Clang -O2 |          158 |          347 |        505 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    934 us
  parse       bench.c:    193 us
  typecheck   bench.c:      6 us
  codegen     bench.c:    134 us
  link        bench_rcc:  65225 us

RCC -O1:
  preprocess  bench.c:    663 us
  parse       bench.c:    131 us
  typecheck   bench.c:      5 us
  opt         bench.c:     21 us
  codegen     bench.c:    129 us
  link        bench_rcc_o1:  72508 us

RCC -O2:
  preprocess  bench.c:    606 us
  parse       bench.c:    115 us
  typecheck   bench.c:      5 us
  opt         bench.c:     22 us
  codegen     bench.c:    116 us
  link        bench_rcc_o2:  62448 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 391338 us
  parse       sqlite3.c: 200961 us
  typecheck   sqlite3.c:  32052 us
  codegen     sqlite3.c: 141089 us

RCC -O1:
  preprocess  sqlite3.c: 296954 us
  parse       sqlite3.c:  66372 us
  typecheck   sqlite3.c:  17518 us
  opt         sqlite3.c:  30865 us
  codegen     sqlite3.c:  80018 us

RCC -O2:
  preprocess  sqlite3.c: 321234 us
  parse       sqlite3.c:  57563 us
  typecheck   sqlite3.c:  20587 us
  opt         sqlite3.c: 192944 us
  codegen     sqlite3.c:  76415 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |      1288 ms |
| RCC -O1   |       871 ms |
| RCC -O2   |      1110 ms |
| TCC       |       130 ms |
| GCC -O0   |      1627 ms |
| GCC -O2   |     15164 ms |
| Clang -O0 |      1441 ms |
| Clang -O2 |     15455 ms |
