/* GCC Bug #123362 - Invalid conversions from nullptr generate misleading diagnostics
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123362
 */


int main(){(int)nullptr;}

int main(){(float)nullptr;}

// These generate the error messages "aggregate value used where an integer was expected", and "aggregate value used where a floating-point was expected" respectively. The type of nullptr is a scalar type, not an aggregate. When a pointer is used instead, for example (float)(void*)0 the error message generated instead is "pointer value used where a floating-point was expected". For the conversion of nullptr to float I think this message should be used instead, or specifically mentioning null pointer value instead of just pointer value. The conversion of nullptr to int should probably get a new diagnostic since pointers are normally convertible to integers.


