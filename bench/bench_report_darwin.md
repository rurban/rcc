# Darwin RCC Benchmark Results

_Generated: July 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) |
| :-------- | -----------: | -----------: | ---------: |
| RCC       |           74 |          729 |        803 |
| RCC -O1   |           94 |          744 |        838 |
| RCC -O2   |          149 |          807 |        956 |
| TCC       |           64 |          729 |        793 |
| GCC -O0   |          117 |          593 |        710 |
| GCC -O2   |          176 |          340 |        516 |
| Clang -O0 |          122 |          588 |        710 |
| Clang -O2 |          137 |          345 |        482 |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c:    894 us
  parse       bench.c:    164 us
  typecheck   bench.c:      5 us
  codegen     bench.c:    210 us
  link        bench_rcc:  68361 us

RCC -O1:
  preprocess  bench.c:    637 us
  parse       bench.c:    133 us
  typecheck   bench.c:      5 us
  opt         bench.c:     18 us
  codegen     bench.c:    144 us
  link        bench_rcc_o1:  62180 us

RCC -O2:
  preprocess  bench.c:    732 us
  parse       bench.c:    150 us
  typecheck   bench.c:      5 us
  opt         bench.c:     19 us
  codegen     bench.c:    142 us
  link        bench_rcc_o2:  65876 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
[1;31merror:[0m too many macro arguments

RCC -O1:
[1;31merror:[0m too many macro arguments

RCC -O2:
[1;31merror:[0m too many macro arguments
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) |
| :-------- | -----------: |
| TCC       |       154 ms |
| GCC -O0   |      1659 ms |
| GCC -O2   |     17801 ms |
| Clang -O0 |      3265 ms |
| Clang -O2 |     19228 ms |
