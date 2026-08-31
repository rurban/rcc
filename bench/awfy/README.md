# Are-We-Fast-Yet: Richards

A medium-sized runtime (generated-code-quality) benchmark, as opposed
to `bench/run_bench.sh`'s compiler-compile-speed benchmarks.

Source: [rochus-keller/Are-we-fast-yet](https://github.com/rochus-keller/Are-we-fast-yet/tree/main/C),
a C99 port of the ["Are We Fast Yet?"](https://github.com/smarr/are-we-fast-yet)
cross-language benchmark suite. `Richards.c`/`Richards.h`/`Benchmark.c`/`Benchmark.h`
are used unmodified from upstream; `main.c` is a small local driver
that runs just this one benchmark (upstream's own `Run.c`/`main.c`
hard-wire all 14 suite benchmarks together, which would require
pulling in every other benchmark's sources just to link).

Richards simulates an OS task scheduler (originally Mario Wolczko's
Smalltalk/Java benchmark) — allocation- and pointer-chasing-heavy,
exercising structs, function-pointer dispatch and linked lists.

## Usage

```
./bench/awfy/run.sh [rcc-binary] [iterations]
```

Runs the benchmark built with rcc at `-O0`/`-O1`/`-O2`, then with gcc
`-O2` as a generated-code-speed baseline.

## License

Richards.c/Richards.h/Benchmark.c/Benchmark.h: MIT, Copyright (c)
2024 Rochus Keller (C99 port); original algorithm derived from Mario
Wolczko's Richards benchmark.
