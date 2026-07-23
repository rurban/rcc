# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           80 |          688 |        768 |
| RCC -O1   |           55 |          636 |        691 |
| RCC -O2   |           57 |          631 |        688 |
| TCC       |           43 |          564 |        607 |
| GCC -O0   |           96 |          569 |        665 |
| GCC -O2   |          227 |          325 |        552 |
| Clang -O0 |          103 |          484 |        587 |
| Clang -O2 |          111 |          290 |        401 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    724 us
  parse       bench.c:    134 us
  typecheck   bench.c:      4 us
  codegen     bench.c:    129 us
  link        bench_rcc:  70065 us

RCC -O1:
  preprocess  bench.c:    699 us
  parse       bench.c:    157 us
  typecheck   bench.c:      4 us
  opt         bench.c:     16 us
  codegen     bench.c:    144 us
  link        bench_rcc_o1:  52589 us

RCC -O2:
  preprocess  bench.c:    550 us
  parse       bench.c:    124 us
  typecheck   bench.c:      5 us
  opt         bench.c:     16 us
  codegen     bench.c:    108 us
  link        bench_rcc_o2:  62562 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c: 246865 us
  parse       sqlite3.c:  56899 us
  typecheck   sqlite3.c:  21793 us
  codegen     sqlite3.c: 132708 us

RCC -O1:
  preprocess  sqlite3.c: 390416 us
  parse       sqlite3.c:  73755 us
  typecheck   sqlite3.c:  17192 us
  opt         sqlite3.c:  20971 us
  codegen     sqlite3.c:  52475 us

RCC -O2:
  preprocess  sqlite3.c: 230303 us
  parse       sqlite3.c:  48243 us
  typecheck   sqlite3.c:  16621 us
  opt         sqlite3.c: 144524 us
  codegen     sqlite3.c:  57145 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| RCC       |       705 ms |
| RCC -O1   |       549 ms |
| RCC -O2   |      1017 ms |
| TCC       |       113 ms |
| GCC -O0   |      1330 ms |
| GCC -O2   |     11120 ms |
| Clang -O0 |      1114 ms |
| Clang -O2 |     10796 ms |
