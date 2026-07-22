/* A function-like macro's actual-argument list at a call site was capped
 * at a hardcoded 32 entries in the preprocessor's argument-collection loop
 * (src/preprocess.c, expand_token()) — hitting a 33rd comma at paren-depth
 * 1 aborted with a bare "too many macro arguments" and no source location.
 * That cap applies even to a macro *defined* as fully variadic
 * ("#define PARAMS(args...) args", taking no fixed parameters at all), so
 * there is no inherent reason for a 32-argument ceiling: the macro is
 * meant to accept and re-emit any number of tokens.
 *
 * Found via a real Linux kernel build: mm/oom_kill.c includes
 * include/trace/events/oom.h, whose TRACE_EVENT(...) invocations expand to
 * "DECLARE_TRACE_EVENT(name, PARAMS(proto), PARAMS(args))". The "proto"
 * macro parameter is itself the literal tokens "TP_PROTO(field1, field2,
 * ...)"; because macro-argument prescan fully expands TP_PROTO(args...)
 * before substituting it into PARAMS(proto), PARAMS ends up invoked with
 * the *bare* comma-separated field list as its actual argument list —
 * easily exceeding 32 commas for a tracepoint with many fields, well
 * before it's re-joined into a single result. Fixed by raising the
 * call-site argument capacity (MAX_CALL_ARGS) from 32 to 128, matching the
 * C99-recommended minimum translation limit for macro parameters/arguments.
 */
#define PARAMS(args...) args
#define TP_PROTO(args...) args
#define WRAP(proto) PARAMS(proto)

int arr[] = {
    WRAP(TP_PROTO(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                  31, 32, 33, 34, 35))
};

int main(void)
{
    if (sizeof(arr) / sizeof(arr[0]) != 35) return 1;
    if (arr[0] != 1) return 2;
    if (arr[34] != 35) return 3;
    return 0;
}
