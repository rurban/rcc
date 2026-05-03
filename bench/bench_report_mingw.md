# Windows RCC Benchmark Results

_Generated: 05/03/2026 10:53:32_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9556 |          758 |      10314 |
| RCC -O1 (optimized)   |         1290 |          632 |       1922 |
| TCC (Tiny C Compiler) |         1290 |          511 |       1801 |
| GCC -O0 (no opt)      |         1270 |          470 |       1740 |
| GCC -O2 (optimized)   |         1278 |          119 |       1397 |
| CLANG -O2 (optimized) |         1268 |          189 |       1457 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 7.41x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 5.73x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
