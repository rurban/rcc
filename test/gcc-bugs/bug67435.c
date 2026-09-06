/* GCC Bug #67435 - Feature request: Implement align-loops attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67435
 */
/* { dg-do compile } */

// Reporter's original problem (comment #0): a performance-critical decoder
// function "fdec" was affected by up to 20% by apparently unrelated changes
// elsewhere in the same translation unit; root cause turned out (comment
// #6) to be that adding/removing code shifts the *instruction alignment* of
// a hot loop.  "-falign-loops=32" fixed it, but that is a whole-program
// command-line option, not something that can be requested for a single
// loop or function from within the source (comment #6):
//
//   #pragma GCC optimize ("align-loops=32")
// or
//   __attribute__((optimize("align-loops=32")))
//
// Neither of these achieves the desired effect (comment #6/#8): there is no
// way to precisely align just the handful of hot loops that matter, without
// forcing alignment (and the code-size cost that comes with it) on the
// whole program via the command line.  This is the feature being
// requested: a per-loop/per-function align-loops attribute.

#pragma GCC optimize ("align-loops=32")

int
fdec (int *p, int n)
{
  int sum = 0;
  for (int i = 0; i < n; i++)
    sum += p[i];
  return sum;
}

__attribute__((optimize("align-loops=32")))
int
fdec2 (int *p, int n)
{
  int sum = 0;
  for (int i = 0; i < n; i++)
    sum += p[i];
  return sum;
}

// Comment #7 additionally quoted gcc's own i386.c default per-CPU loop
// alignment table for reference (not part of the reporter's test case, just
// context showing which -march targets already default to 32-byte loop
// alignment):
//
//  2540 /* Processor target table, indexed by processor number */
//  2541 struct ptt
//  2542 {
//  2543   const char *const name;             /* processor name  */
//  2544   const struct processor_costs *cost; /* Processor costs */
//  2545   const int align_loop;               /* Default alignments.  */
//  2546   const int align_loop_max_skip;
//  2547   const int align_jump;
//  2548   const int align_jump_max_skip;
//  2549   const int align_func;
//  2550 };
//  2551
//  2552 /* This table must be in sync with enum processor_type in i386.h.  */
//  2553 static const struct ptt processor_target_table[PROCESSOR_max] =
//  2554 {
//  2555   {"generic", &generic_cost, 16, 10, 16, 10, 16},
//  2556   {"i386", &i386_cost, 4, 3, 4, 3, 4},
//  2557   {"i486", &i486_cost, 16, 15, 16, 15, 16},
//  2558   {"pentium", &pentium_cost, 16, 7, 16, 7, 16},
//  2559   {"iamcu", &iamcu_cost, 16, 7, 16, 7, 16},
//  2560   {"pentiumpro", &pentiumpro_cost, 16, 15, 16, 10, 16},
//  2561   {"pentium4", &pentium4_cost, 0, 0, 0, 0, 0},
//  2562   {"nocona", &nocona_cost, 0, 0, 0, 0, 0},
//  2563   {"core2", &core_cost, 16, 10, 16, 10, 16},
//  2564   {"nehalem", &core_cost, 16, 10, 16, 10, 16},
//  2565   {"sandybridge", &core_cost, 16, 10, 16, 10, 16},
//  2566   {"haswell", &core_cost, 16, 10, 16, 10, 16},
//  2567   {"bonnell", &atom_cost, 16, 15, 16, 7, 16},
//  2568   {"silvermont", &slm_cost, 16, 15, 16, 7, 16},
//  2569   {"knl", &slm_cost, 16, 15, 16, 7, 16},
//  2570   {"intel", &intel_cost, 16, 15, 16, 7, 16},
//  2571   {"geode", &geode_cost, 0, 0, 0, 0, 0},
//  2572   {"k6", &k6_cost, 32, 7, 32, 7, 32},
//  2573   {"athlon", &athlon_cost, 16, 7, 16, 7, 16},
//  2574   {"k8", &k8_cost, 16, 7, 16, 7, 16},
//  2575   {"amdfam10", &amdfam10_cost, 32, 24, 32, 7, 32},
//  2576   {"bdver1", &bdver1_cost, 16, 10, 16, 7, 11},
//  2577   {"bdver2", &bdver2_cost, 16, 10, 16, 7, 11},
//  2578   {"bdver3", &bdver3_cost, 16, 10, 16, 7, 11},
//  2579   {"bdver4", &bdver4_cost, 16, 10, 16, 7, 11},
//  2580   {"btver1", &btver1_cost, 16, 10, 16, 7, 11},
//  2581   {"btver2", &btver2_cost, 16, 10, 16, 7, 11}
//  2582 };
//
// Only AMD's k6 and amdfam10 default to align_loop=32; most other targets
// default to 16, which is why the reporter had to request 32 explicitly.
// NOTE: this bug has no minimal single-TU reproducer that observably
// demonstrates the missing per-loop attribute (the effect is on generated
// instruction alignment, visible only via disassembly, not via compile or
// runtime behavior); the two functions above compile the exact pragma/
// attribute forms the reporter tried, which is the closest faithful,
// standalone approximation of the feature request.
