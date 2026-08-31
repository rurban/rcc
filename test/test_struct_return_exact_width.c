/* A struct/union returned by value in GP registers (SysV/AAPCS64: <=16
 * bytes, all-integer members; Win64: <=8 bytes) was stored back to memory
 * using a store width rounded UP to 4 or 8 bytes regardless of the
 * struct's actual size -- `int sz = ty->size <= 4 ? 4 : 8;`. For any
 * struct smaller than 4 bytes (1, 2, or 3 bytes) this overwrites 1-3
 * bytes PAST the struct's own end with whatever garbage was left in the
 * register's high bits.
 *
 * Found via a real CPython build: `_Py_BackoffCounter advance_backoff_
 * counter(_Py_BackoffCounter)` returns a 2-byte struct (one uint16_t
 * field), assigned back into a `_Py_CODEUNIT` bytecode array slot
 * (`this_instr[1].counter = advance_backoff_counter(...)`). The 4-byte
 * store clobbered the *next* bytecode instruction's opcode+arg byte
 * (adjacent in the tightly-packed array), corrupting it to opcode 0
 * (CACHE) and crashing the interpreter with "Fatal Python error:
 * _PyEval_EvalFrameDefault: Executing a cache."
 *
 * Also covers the inverse regression this fix's first attempt
 * introduced: a `_Complex char`/`_Complex short` (<=8 bytes total) packs
 * BOTH real and imaginary parts into a single register -- storing only
 * one component's width there would silently drop the imaginary part.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint16_t value_and_backoff;
} Counter;

static Counter advance(Counter c) {
    c.value_and_backoff = (uint16_t)(c.value_and_backoff + 1);
    return c;
}

typedef union {
    uint16_t cache;
    struct {
        uint8_t code;
        uint8_t arg;
    } op;
    Counter counter;
} CodeUnit;

typedef struct {
    uint8_t v;
} Byte1;

static Byte1 make_byte1(uint8_t v) {
    Byte1 b;
    b.v = v;
    return b;
}

int main(void) {
    int fail = 0;

    /* 2-byte struct return into a packed array slot: must not touch the
     * adjacent code unit. */
    CodeUnit arr[2];
    arr[0].cache = 5;
    arr[1].op.code = 112; /* arbitrary nonzero marker, like STORE_FAST */
    arr[1].op.arg = 7;
    arr[0].counter = advance(arr[0].counter);
    if (arr[0].counter.value_and_backoff != 6) {
        printf("FAIL: 2-byte struct return value wrong: got %u want 6\n",
               (unsigned)arr[0].counter.value_and_backoff);
        fail = 1;
    }
    if (arr[1].op.code != 112 || arr[1].op.arg != 7) {
        printf("FAIL: 2-byte struct return overran into adjacent memory: "
               "code=%u arg=%u (want 112/7)\n",
               (unsigned)arr[1].op.code, (unsigned)arr[1].op.arg);
        fail = 1;
    }

    /* 1-byte struct return: same overrun risk, packed into a byte array. */
    uint8_t bytes[3] = {0, 0xAB, 0xCD};
    Byte1 b = make_byte1(0x11);
    memcpy(&bytes[0], &b, sizeof(b));
    if (bytes[0] != 0x11 || bytes[1] != 0xAB || bytes[2] != 0xCD) {
        printf("FAIL: 1-byte struct return corrupted adjacent bytes: "
               "%02x %02x %02x (want 11 ab cd)\n",
               bytes[0], bytes[1], bytes[2]);
        fail = 1;
    }

    /* _Complex char / _Complex short: both components must survive a
     * <=8-byte packed-register return (the fix's own regression risk). */
    {
        _Complex char cc = 3 + 5i;
        if (__real__ cc != 3 || __imag__ cc != 5) {
            printf("FAIL: _Complex char return: real=%d imag=%d (want 3/5)\n",
                   (int)__real__ cc, (int)__imag__ cc);
            fail = 1;
        }
    }
    {
        _Complex short cs = 100 + 200i;
        if (__real__ cs != 100 || __imag__ cs != 200) {
            printf("FAIL: _Complex short return: real=%d imag=%d (want 100/200)\n",
                   (int)__real__ cs, (int)__imag__ cs);
            fail = 1;
        }
    }

    if (fail) return 1;
    printf("OK small-struct/complex return values use exact store width\n");
    return 0;
}
