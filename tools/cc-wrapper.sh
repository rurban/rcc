#!/bin/sh
# rcc is native-only: one binary targets one word width, by design (see
# AGENTS.md). A handful of always-built x86_64 kernel pieces genuinely need a
# *different* word width in the same build — the vdso32 compat vDSO (-m32,
# gated behind CONFIG_IA32_EMULATION/CONFIG_COMPAT_32 — see linux.sh, which
# disables it) and arch/x86/realmode/rm/'s 16-bit AP-wakeup/ACPI-resume
# trampoline (-m16, unconditional: arch/x86/Kbuild's "obj-y += realmode/",
# no Kconfig gate to turn it off). Route just those compiler invocations to
# the real, multilib-capable host gcc; everything else still goes to rcc.
pwd="$(dirname "$0")"
need_gcc=
for arg in "$@"; do
    case "$arg" in
        -m16|-m32) need_gcc=1 ;;
    esac
done
if [ -n "$need_gcc" ]; then
    # rcc accepts (with a warning, not an error) a Clang/MS-compat flag
    # kbuild probes for via cc-option against $(CC) — real gcc rejects it
    # outright, so it must be stripped before reaching it. Kernel compiler
    # flags never contain embedded whitespace, so IFS-splitting the
    # filtered, one-per-line list back into positional params is safe.
    # shellcheck disable=SC2046
    set -- $(printf '%s\n' "$@" | grep -vFx -- "-fms-anonymous-structs")
    exec gcc "$@"
fi
exec "$pwd"/../rcc "$@"
