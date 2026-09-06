/* GCC Bug #114011 - Feature request: __goto__
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114011
 */


enum { DISALLOW_GOTO_HERE = 0 }; //normally, goto is allowed
#define goto while(_Generic((int(*)[!DISALLOW_GOTO_HERE])0, int(*)[1]:1)) goto //statically checked goto
int main(void){
//     goto next; next:; //OK, not disallowed in this context

    #if 0 //would fail to compile
//     enum {DISALLOW_GOTO_HERE=1}; //disallowed in this context
    goto next2; next2:;
    #endif
}

// While this redefine does not syntactically disturb C, it does disturb `__asm goto()`, which I, unfortunately, have one very frequently used instance of, and since there's no way to suppress an object macro redefine, I'd like to be able to change it to `__asm __goto__` and have it peacefully coexist with the goto redefine.
// )


