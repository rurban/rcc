# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           62 |          675 |        737 |
| RCC -O1   |           55 |          623 |        678 |
| RCC -O2   |           71 |          630 |        701 |
| TCC       |           39 |          546 |        585 |
| GCC -O0   |           74 |          455 |        529 |
| GCC -O2   |          116 |          263 |        379 |
| Clang -O0 |           62 |          442 |        504 |
| Clang -O2 |           95 |          269 |        364 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    752 us
  parse       bench.c:    140 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    132 us
  link        bench_rcc:  59015 us

RCC -O1:
  preprocess  bench.c:    625 us
  parse       bench.c:    139 us
  typecheck   bench.c:      5 us
  opt         bench.c:     22 us
  codegen     bench.c:    119 us
  link        bench_rcc_o1:  55092 us
```

RCC -O2:
preprocess bench.c: 615 us
parse bench.c: 119 us
typecheck bench.c: 5 us
opt bench.c: 19 us
codegen bench.c: 127 us
link bench_rcc_o2: 52490 us

```

## RCC Substep Timing -- sqlite3.c

```

RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

```

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |        93 ms |
| GCC -O0   |       926 ms |
| GCC -O2   |     10361 ms |
| Clang -O0 |       979 ms |
| Clang -O2 |     10347 ms |
