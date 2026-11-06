#!/bin/sh
# Probe for the libdwarf producer library (libdwarfp) + headers. Prints 1
# when a test program compiles and links, else 0. Lives in a script because
# inline $(shell ...) probes with spaces break BSD makes. Args: CC, CFLAGS, LIBS.
CC="${1:-cc}"
# CFLAGS/LIBS are intentionally word-split: they carry multiple flags.
# shellcheck disable=SC2086
printf '\043include <libdwarf.h>\n\043include <libdwarfp.h>\nint main(void){Dwarf_P_Debug d=0;Dwarf_Error e=0;return dwarf_producer_init(0,0,0,0,0,"x86_64","V4",0,&d,&e);}\n' |
    $CC $CFLAGS -x c - -o /dev/null $LIBS >/dev/null 2>&1 && echo 1 || echo 0
