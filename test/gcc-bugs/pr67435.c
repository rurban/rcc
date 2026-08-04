/* GCC Bug #67435 - Feature request: Implement align-loops attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67435
 */


2542 {                                                                                                                                                                       
 2550 };                                                                                                                                                                      
 2554 {                                                                                                                                                                       
 2555   {"generic", &generic_cost, 16, 10, 16, 10, 16},                                                                                                                       
 2556   {"i386", &i386_cost, 4, 3, 4, 3, 4},                                                                                                                                  
 2557   {"i486", &i486_cost, 16, 15, 16, 15, 16},                                                                                                                             
 2558   {"pentium", &pentium_cost, 16, 7, 16, 7, 16},                                                                                                                         
 2559   {"iamcu", &iamcu_cost, 16, 7, 16, 7, 16},                                                                                                                             
 2560   {"pentiumpro", &pentiumpro_cost, 16, 15, 16, 10, 16},                                                                                                                 
 2561   {"pentium4", &pentium4_cost, 0, 0, 0, 0, 0},                                                                                                                          
 2562   {"nocona", &nocona_cost, 0, 0, 0, 0, 0},                                                                                                                              
 2563   {"core2", &core_cost, 16, 10, 16, 10, 16},                                                                                                                            
 2564   {"nehalem", &core_cost, 16, 10, 16, 10, 16},                                                                                                                          
 2565   {"sandybridge", &core_cost, 16, 10, 16, 10, 16},                                                                                                                      
 2566   {"haswell", &core_cost, 16, 10, 16, 10, 16},                                                                                                                          
 2567   {"bonnell", &atom_cost, 16, 15, 16, 7, 16},                                                                                                                           
 2568   {"silvermont", &slm_cost, 16, 15, 16, 7, 16},                                                                                                                         
 2569   {"knl", &slm_cost, 16, 15, 16, 7, 16},                                                                                                                                
 2570   {"intel", &intel_cost, 16, 15, 16, 7, 16},                                                                                                                            
 2571   {"geode", &geode_cost, 0, 0, 0, 0, 0},                                                                                                                                
 2572   {"k6", &k6_cost, 32, 7, 32, 7, 32},                                                                                                                                   
 2573   {"athlon", &athlon_cost, 16, 7, 16, 7, 16},                                                                                                                           
 2574   {"k8", &k8_cost, 16, 7, 16, 7, 16},                                                                                                                                   
 2575   {"amdfam10", &amdfam10_cost, 32, 24, 32, 7, 32},                                                                                                                      
 2576   {"bdver1", &bdver1_cost, 16, 10, 16, 7, 11},                                                                                                                          
 2577   {"bdver2", &bdver2_cost, 16, 10, 16, 7, 11},                                                                                                                          
 2578   {"bdver3", &bdver3_cost, 16, 10, 16, 7, 11},                                                                                                                          
 2579   {"bdver4", &bdver4_cost, 16, 10, 16, 7, 11},                                                                                                                          
 2580   {"btver1", &btver1_cost, 16, 10, 16, 7, 11},                                                                                                                          
 2581   {"btver2", &btver2_cost, 16, 10, 16, 7, 11}                                                                                                                           
 2582 };


