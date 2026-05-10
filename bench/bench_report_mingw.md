# Windows RCC Benchmark Results

_Generated: 05/10/2026 12:01:34_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        14673 |          555 |      15228 |
| RCC -O1 (optimized)   |         1251 |          542 |       1793 |
| TCC (Tiny C Compiler) |         1260 |          427 |       1687 |
| GCC -O0 (no opt)      |         4277 |          416 |       4693 |
| GCC -O2 (optimized)   |         5265 |          115 |       5380 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 11.65x
- Execute speed : RCC/TCC = 1.3x
- Total : RCC/TCC = 9.03x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
