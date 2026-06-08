# Windows RCC Benchmark Results

_Generated: 06/08/2026 11:23:13_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3814 |          607 |       4421 |
| RCC -O1 (optimized)   |         1256 |          609 |       1865 |
| TCC (Tiny C Compiler) |         1285 |          437 |       1722 |
| GCC -O0 (no opt)      |         1282 |          419 |       1701 |
| GCC -O2 (optimized)   |         1260 |          117 |       1377 |
| CLANG -O2 (optimized) |         1256 |          170 |       1426 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.97x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 2.57x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
