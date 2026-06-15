// from https://en.cppreference.com/c/language/attributes
/*
  __has_c_attribute can be expanded in the expression of #if and
  #elif. It is treated as a defined macro by #ifdef, #ifndef and
  defined but cannot be used anywhere else.

attribute-token Attribute 	Value 	 Standard
deprecated 	[[deprecated]] 	201904L  (C23)
fallthrough 	[[fallthrough]] 201904L  (C23)
maybe_unused 	[[maybe_unused]] 201904L (C23)
nodiscard 	[[nodiscard]] 	202003L  (C23)
noreturn        [[noreturn]]    202202L  (C23)
_Noreturn 	[[_Noreturn]] 	202202L  (C23)  **deprecated**
unsequenced 	[[unsequenced]] 202207L  (C23)
reproducible 	[[reproducible]] 202207L (C23)

[[nodiscard("reason")]]
[[deprecated("reason")]]
 */

#if __has_c_attribute(nodiscard) < 202003L
#error "__has_c_attribute(nodiscard)"
#endif

[[gnu::hot]] [[gnu::const]] [[nodiscard]]
int f(void); // declare f with three attributes

[[gnu::const, gnu::hot, nodiscard("reason")]]
int f(void); // the same as above, but uses a single attr
             // specifier that contains three attributes

int f(void) { return 0; }

int main(void)
{
}
