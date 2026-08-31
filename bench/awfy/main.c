/* Minimal driver for the "Are We Fast Yet?" Richards benchmark
 * (https://github.com/rochus-keller/Are-we-fast-yet/tree/main/C),
 * adapted from that project's own Run.c/main.c harness but wired up
 * for just this one benchmark instead of pulling in all 14 (which
 * would require every other benchmark's sources to link).
 *
 * Richards is the classic OS-process-scheduler-simulation benchmark
 * (originally Mario Wolczko's Smalltalk/Java version) -- a medium-
 * sized, allocation- and pointer-chasing-heavy workload exercising
 * structs, function pointers and linked data structures, distinct
 * from bench/bench.c's smaller integer/float microbenchmarks.
 */
#include "Richards.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    int numIterations = argc > 1 ? atoi(argv[1]) : 100;
    int innerIterations = argc > 2 ? atoi(argv[2]) : 1;
    long total = 0;

    printf("Starting Richards benchmark (iterations=%d, inner=%d)...\n",
           numIterations, innerIterations);

    for (int i = 0; i < numIterations; i++) {
        Benchmark *bench = Richards_create();
        struct timeval start, end;

        gettimeofday(&start, 0);
        bool (*innerBenchmarkLoop)(Benchmark *, int) = Benchmark_innerBenchmarkLoop;
        if (bench->innerBenchmarkLoop)
            innerBenchmarkLoop = bench->innerBenchmarkLoop;
        if (!innerBenchmarkLoop(bench, innerIterations)) {
            fprintf(stderr, "Richards benchmark failed with incorrect result\n");
            if (bench->dispose) bench->dispose(bench);
            free(bench);
            return 1;
        }
        gettimeofday(&end, 0);

        if (bench->dispose) bench->dispose(bench);
        free(bench);

        long seconds = end.tv_sec - start.tv_sec;
        long microseconds = end.tv_usec - start.tv_usec;
        total += seconds * 1000000 + microseconds;
    }

    printf("Richards: iterations=%d average: %ld us total: %ld us\n",
           numIterations, total / numIterations, total);
    return 0;
}
