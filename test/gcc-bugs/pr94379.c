/* GCC Bug #94379 - Feature request: like clang, support __attribute((__warn_unused_result__)) on structs, unions, and enums
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94379
 */


typedef struct __attribute__((__warn_unused_result__)) aStructType{ int x; } aStructType;
aStructType getStruct(void);

typedef union __attribute__((__warn_unused_result__)) aUnionType{ int x; } aUnionType;
aUnionType getUnion(void);

typedef enum __attribute__((__warn_unused_result__)) anEnumType{ anEnumarationConstant } anEnumType;
anEnumType getEnum(void);

int main()
{
// 	getEnum();
// 	getStruct();
// 	getUnion();
}

// (along with its current un-void-able warn_unused_result (<a href="https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425">https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425</a>)).


