#ifndef _STDATOMIC_H
#define _STDATOMIC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST,
} memory_order;

#define atomic_store_explicit(ptr, val, order) __atomic_store_n(ptr, val, order)
#define atomic_store(object, desired) \
    atomic_store_explicit(object, desired, __ATOMIC_SEQ_CST)

#define atomic_load_explicit(ptr, order) __atomic_load_n(ptr, order)
#define atomic_load(object) \
    atomic_load_explicit(object, __ATOMIC_SEQ_CST)

#define atomic_exchange_explicit(ptr, val, order) __atomic_exchange_n(ptr, val, order)
#define atomic_exchange(object, desired) \
    atomic_exchange_explicit(object, desired, __ATOMIC_SEQ_CST)

#define atomic_compare_exchange_strong_explicit(ptr, expected, desired, s, f) \
    __atomic_compare_exchange_n(ptr, expected, desired, 0, s, f)
#define atomic_compare_exchange_strong(object, expected, desired) \
    atomic_compare_exchange_strong_explicit(object, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_weak_explicit(ptr, expected, desired, s, f) \
    __atomic_compare_exchange_n(ptr, expected, desired, 1, s, f)
#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_weak_explicit(object, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define atomic_fetch_add_explicit(ptr, val, order) __atomic_fetch_add(ptr, val, order)
#define atomic_fetch_add(object, operand) \
    atomic_fetch_add_explicit(object, operand, __ATOMIC_SEQ_CST)

#define atomic_fetch_sub_explicit(ptr, val, order) __atomic_fetch_sub(ptr, val, order)
#define atomic_fetch_sub(object, operand) \
    atomic_fetch_sub_explicit(object, operand, __ATOMIC_SEQ_CST)

#define atomic_fetch_or_explicit(ptr, val, order) __atomic_fetch_or(ptr, val, order)
#define atomic_fetch_or(object, operand) \
    atomic_fetch_or_explicit(object, operand, __ATOMIC_SEQ_CST)

#define atomic_fetch_xor_explicit(ptr, val, order) __atomic_fetch_xor(ptr, val, order)
#define atomic_fetch_xor(object, operand) \
    atomic_fetch_xor_explicit(object, operand, __ATOMIC_SEQ_CST)

#define atomic_fetch_and_explicit(ptr, val, order) __atomic_fetch_and(ptr, val, order)
#define atomic_fetch_and(object, operand) \
    atomic_fetch_and_explicit(object, operand, __ATOMIC_SEQ_CST)

#define atomic_thread_fence(order) __atomic_thread_fence(order)
#define atomic_signal_fence(order) __atomic_signal_fence(order)
#define atomic_is_lock_free(obj) __atomic_is_lock_free(sizeof(*(obj)), obj)

#define atomic_flag_test_and_set_explicit(ptr, order) __atomic_test_and_set(ptr, order)
#define atomic_flag_test_and_set(ptr) \
    atomic_flag_test_and_set_explicit(ptr, __ATOMIC_SEQ_CST)
#define atomic_flag_clear_explicit(ptr, order) __atomic_clear(ptr, order)
#define atomic_flag_clear(ptr) \
    atomic_flag_clear_explicit(ptr, __ATOMIC_SEQ_CST)

#define atomic_init(object, desired) atomic_store_explicit(object, desired, __ATOMIC_RELAXED)

#define ATOMIC_FLAG_INIT {0}
#define ATOMIC_VAR_INIT(value) (value)

typedef _Atomic(_Bool) atomic_bool;
typedef struct {
    atomic_bool _Value;
} atomic_flag;
typedef _Atomic(char) atomic_char;
typedef _Atomic(signed char) atomic_schar;
typedef _Atomic(unsigned char) atomic_uchar;
typedef _Atomic(short) atomic_short;
typedef _Atomic(unsigned short) atomic_ushort;
typedef _Atomic(int) atomic_int;
typedef _Atomic(unsigned int) atomic_uint;
typedef _Atomic(long) atomic_long;
typedef _Atomic(unsigned long) atomic_ulong;
typedef _Atomic(long long) atomic_llong;
typedef _Atomic(unsigned long long) atomic_ullong;
typedef _Atomic(size_t) atomic_size_t;
typedef _Atomic(ptrdiff_t) atomic_ptrdiff_t;
typedef _Atomic(intptr_t) atomic_intptr_t;
typedef _Atomic(uintptr_t) atomic_uintptr_t;
typedef _Atomic(intmax_t) atomic_intmax_t;
typedef _Atomic(uintmax_t) atomic_uintmax_t;
typedef _Atomic(uint_least16_t) atomic_char16_t;
typedef _Atomic(uint_least32_t) atomic_char32_t;
typedef _Atomic(wchar_t) atomic_wchar_t;
typedef _Atomic(int_least8_t) atomic_int_least8_t;
typedef _Atomic(uint_least8_t) atomic_uint_least8_t;
typedef _Atomic(int_least16_t) atomic_int_least16_t;
typedef _Atomic(uint_least16_t) atomic_uint_least16_t;
typedef _Atomic(int_least32_t) atomic_int_least32_t;
typedef _Atomic(uint_least32_t) atomic_uint_least32_t;
typedef _Atomic(int_least64_t) atomic_int_least64_t;
typedef _Atomic(uint_least64_t) atomic_uint_least64_t;
typedef _Atomic(int_fast8_t) atomic_int_fast8_t;
typedef _Atomic(uint_fast8_t) atomic_uint_fast8_t;
typedef _Atomic(int_fast16_t) atomic_int_fast16_t;
typedef _Atomic(uint_fast16_t) atomic_uint_fast16_t;
typedef _Atomic(int_fast32_t) atomic_int_fast32_t;
typedef _Atomic(uint_fast32_t) atomic_uint_fast32_t;
typedef _Atomic(int_fast64_t) atomic_int_fast64_t;
typedef _Atomic(uint_fast64_t) atomic_uint_fast64_t;

#define ATOMIC_BOOL_LOCK_FREE __GCC_ATOMIC_BOOL_LOCK_FREE
#define ATOMIC_CHAR_LOCK_FREE __GCC_ATOMIC_CHAR_LOCK_FREE
#define ATOMIC_CHAR16_T_LOCK_FREE __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define ATOMIC_CHAR32_T_LOCK_FREE __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define ATOMIC_WCHAR_T_LOCK_FREE __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define ATOMIC_SHORT_LOCK_FREE __GCC_ATOMIC_SHORT_LOCK_FREE
#define ATOMIC_INT_LOCK_FREE __GCC_ATOMIC_INT_LOCK_FREE
#define ATOMIC_LONG_LOCK_FREE __GCC_ATOMIC_LONG_LOCK_FREE
#define ATOMIC_LLONG_LOCK_FREE __GCC_ATOMIC_LLONG_LOCK_FREE
#define ATOMIC_POINTER_LOCK_FREE __GCC_ATOMIC_POINTER_LOCK_FREE

#define kill_dependency(y) (y)

#if __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDATOMIC_H__ 202311L
#endif
