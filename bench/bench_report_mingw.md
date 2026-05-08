# Windows RCC Benchmark Results

_Generated: 05/08/2026 14:44:07_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6501 |          556 |       7057 |
| RCC -O1 (optimized)   |         1277 |          556 |       1833 |
| TCC (Tiny C Compiler) |         1275 |          455 |       1730 |
| GCC -O0 (no opt)      |         4315 |          418 |       4733 |
| GCC -O2 (optimized)   |         5316 |          128 |       5444 |
| CLANG -O2 (optimized) |         9126 |          172 |       9298 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.1x
- Execute speed : RCC/TCC = 1.22x
- Total : RCC/TCC = 4.08x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
