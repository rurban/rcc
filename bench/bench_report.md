# Linux RCC Benchmark Results

_Generated: September 2026_

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |           14 |          731 |        745 |     0% |
| RCC -O1   |           14 |          702 |        716 |     4% |
| RCC -O2   |           14 |          700 |        714 |     7% |
| TCC       |            5 |          485 |        490 |     1% |
| SLIMCC    |           31 |          499 |        530 |     0% |
| XCC       |            8 |          357 |        365 |     0% |
| KEFIR     |          176 |          570 |        746 |     1% |
| KEFIR -O1 |          184 |          306 |        490 |     1% |
| SCC       |           32 |          614 |        646 |     3% |
| LACC      |           21 |          754 |        775 |     4% |
| ANTCC     |           23 |          411 |        434 |     4% |
| CAKE      |           89 |          477 |        566 |     1% |
| BCC       |           28 |          557 |        585 |     0% |
| CCC       |           32 |          533 |        565 |    14% |
| GCC -O0   |           53 |          478 |        531 |     3% |
| GCC -O2   |          144 |          174 |        318 |     1% |
| Clang -O0 |           78 |          461 |        539 |     1% |
| Clang -O2 |          124 |          176 |        300 |     1% |

## Are-We-Fast-Yet Suite (14 benchmarks)

| Compiler  | Compile (ms) | Execute (ms) | Total (ms) | Spread |
| :-------- | -----------: | -----------: | ---------: | -----: |
| RCC       |          274 |         5799 |       6073 |     1% |
| RCC -O1   |          276 |         6567 |       6843 |     1% |
| RCC -O2   |          278 |         6109 |       6387 |     0% |
| TCC       |           65 |         5034 |       5099 |     3% |
| SLIMCC    |          248 |         4914 |       5162 |     1% |
| KEFIR     |         4379 |         5274 |       9653 |     0% |
| KEFIR -O1 |         4840 |         3677 |       8517 |     0% |
| ANTCC     |          183 |         3709 |       3892 |     2% |
| CCC       |          851 |         4456 |       5307 |     1% |
| BCC       |         2954 |         4677 |       7631 |     0% |
| GCC -O0   |          831 |         4827 |       5658 |     1% |
| GCC -O2   |         1835 |         2521 |       4356 |     0% |
| Clang -O0 |          853 |         4571 |       5424 |     0% |
| Clang -O2 |         1577 |         2557 |       4134 |     0% |

## RCC Substep Timing

```
RCC:
  preprocess  bench.c       :   9737 us
  parse       bench.c       :    615 us
  typecheck   bench.c       :      4 us
  codegen     bench.c       :    291 us
  link        bench_rcc     :    813 us

RCC -O1:
  preprocess  bench.c       :  10036 us
  parse       bench.c       :    583 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :     93 us
  codegen     bench.c       :    352 us
  link        bench_o1      :    828 us

RCC -O2:
  preprocess  bench.c       :  10352 us
  parse       bench.c       :    623 us
  typecheck   bench.c       :      4 us
  opt         bench.c       :    109 us
  codegen     bench.c       :    306 us
  link        bench_o2      :    876 us
```

## RCC Substep Timing -- sqlite3.c

```
RCC:
  preprocess  sqlite3.c     : 250750 us
  parse       sqlite3.c     : 152411 us
  typecheck   sqlite3.c     :   8456 us
  codegen     sqlite3.c     : 201657 us
  link        sqlite3.so    :   8539 us

RCC -O1:
  preprocess  sqlite3.c     : 246717 us
  parse       sqlite3.c     : 148902 us
  typecheck   sqlite3.c     :   8154 us
  opt         sqlite3.c     :  65164 us
  codegen     sqlite3.c     : 239606 us
  link        sqlite3.so    :   9366 us

RCC -O2:
  preprocess  sqlite3.c     : 247811 us
  parse       sqlite3.c     : 149025 us
  typecheck   sqlite3.c     :   8337 us
  opt         sqlite3.c     :  74302 us
  codegen     sqlite3.c     : 242058 us
  link        sqlite3.so    :   9272 us
```

## Large File Compile-Only (sqlite3.c)

| Compiler  | Compile (ms) | Spread |
| :-------- | -----------: | -----: |
| RCC       |       718 ms |     1% |
| RCC -O1   |       792 ms |     0% |
| RCC -O2   |       806 ms |     0% |
| TCC       |        96 ms |     1% |
| SLIMCC    |       646 ms |     1% |
| KEFIR     |     18266 ms |     0% |
| KEFIR -O1 |     33044 ms |     0% |
| ANTCC     |       389 ms |     0% |
| CCC       |     12637 ms |     0% |
| GCC -O0   |      4033 ms |     0% |
| GCC -O2   |     25536 ms |     0% |
| Clang -O0 |      1788 ms |     0% |
| Clang -O2 |     20377 ms |     7% |
