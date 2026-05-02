# Windows RCC Benchmark Results

_Generated: 05/02/2026 07:10:22_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3049 |          553 |       3602 |
| RCC -O1 (optimized)   |         1245 |          550 |       1795 |
| TCC (Tiny C Compiler) |         1254 |          428 |       1682 |
| GCC -O0 (no opt)      |         1241 |          420 |       1661 |
| GCC -O2 (optimized)   |         1237 |          115 |       1352 |
| CLANG -O2 (optimized) |         1247 |          169 |       1416 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.43x
- Execute speed : RCC/TCC = 1.29x
- Total : RCC/TCC = 2.14x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
