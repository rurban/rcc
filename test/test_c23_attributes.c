// from https://en.cppreference.com/c/language/attributes
/*
  __has_c_attribute can be expanded in the expression of #if and
  #elif. It is treated as a defined macro by #ifdef, #ifndef and
  defined but cannot be used anywhere else.

attribute-token Attribute       Value    Standard
deprecated     [[deprecated]]   201904L  (C23)
fallthrough    [[fallthrough]]  201904L  (C23)
maybe_unused   [[maybe_unused]] 201904L  (C23)
nodiscard      [[nodiscard]]    202003L  (C23)
noreturn       [[noreturn]]     202202L  (C23)
_Noreturn      [[_Noreturn]]    202202L  (C23)  **deprecated**
unsequenced    [[unsequenced]]  202207L  (C23)
reproducible   [[reproducible]] 202207L (C23)

[[nodiscard("reason")]]
[[deprecated("reason")]]
 */

#if __has_c_attribute(nodiscard) < 202003L
#error "__has_c_attribute(nodiscard)"
#endif

#if __has_c_attribute(deprecated) < 201904L
#error "__has_c_attribute(deprecated)"
#endif

#if __has_c_attribute(noreturn) < 202202L
#error "__has_c_attribute(noreturn)"
#endif

// C23 [[noreturn]] attribute — function does not return
[[noreturn]] void exit_now(void);

// C23 _Noreturn keyword (deprecated in favor of [[noreturn]])
_Noreturn void abort_now(void);

// C23 [[nodiscard]] — warn when return value is discarded
[[nodiscard]] int important(void);

// C23 [[nodiscard("reason")]] — with diagnostic message
[[nodiscard("this value must be checked")]] int critical(void);

// C23 [[deprecated]] — warn when used
[[deprecated]] void old_function(void);

// C23 [[deprecated("reason")]] — with message
[[deprecated("use new_function instead")]] void legacy(void);

// C23 [[gnu::hot]] [[gnu::const]] [[nodiscard]] — mixed attrs
[[gnu::hot]] [[gnu::const]] [[nodiscard]]
int f(void);

// C23 comma-separated attributes in single [[...]]
[[gnu::const, gnu::hot, nodiscard("reason")]]
int g(void);

int f(void) { return 42; }
int g(void) { return 99; }
int important(void) { return 1; }
int critical(void) { return 2; }

int main(void)
{
    // [[nodiscard]] functions: using return value is fine (no warning)
    if (important() != 1) return 1;
    if (critical() != 2) return 2;
    if (f() != 42) return 3;
    if (g() != 99) return 4;
    return 0;
}
