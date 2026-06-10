# Windows RCC Benchmark Results

_Generated: 06/10/2026 12:37:07_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2849 |          757 |       3606 |
| RCC -O1 (optimized)   |         1262 |          741 |       2003 |
| TCC (Tiny C Compiler) |         1271 |          493 |       1764 |
| GCC -O0 (no opt)      |         1269 |          472 |       1741 |
| GCC -O2 (optimized)   |         1270 |          119 |       1389 |
| CLANG -O2 (optimized) |         1275 |          189 |       1464 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.24x
- Execute speed : RCC/TCC = 1.54x
- Total : RCC/TCC = 2.04x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
