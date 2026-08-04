/* GCC Bug #121736 - Offsetof with non-constant array index should be diagnosed as an extension
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121736
 */


#include<stddef.h>
int main(){
    int x=2;
    struct y{
        int z[4];
    };
//     offsetof(struct y,z[x]);
}
// This code shouldn't be accepted with -pedantic-errors because it violates the following text:
// <span class="quote">> The macros are
// > 
// > ...
// > 
// > offsetof(type, member-designator)
// > 
// > which expands to an integer constant expression that has type size_t, the
// > value of which is the offset in bytes, to the subobject (designated by
// > member-designator), from the beginning of any object of type type. The type
// > and member designator shall be such that given
// > 
// > static type t;
// > 
// > then the expression &(t.member-designator) evaluates to an address constant.
// > If the specified type name contains a comma not between matching parentheses
// > or if the specified member is a bit-field, the behavior is undefined.</span >
// Section 7.21 "Common definitions <stddef.h>" Paragraph 4 ISO/IEC 9899:2024
// Strictly, the behavior is undefined so not diagnosing this is correct. However, diagnosing it with -pedantic seems simple to do so I think it should be done.
// As an aside, the GCC C++ compiler doesn't support this extension. Clang and MSVC support this extension in both C and C++ (though they also don't diagnose it).


