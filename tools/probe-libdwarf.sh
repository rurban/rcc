#!/bin/sh
# Probe for the libdwarf producer library (libdwarfp) + headers. Prints 1
# when a test program compiles and links, else 0. Lives in a script because
# inline $(shell ...) probes with spaces break BSD makes. Args: CC, CFLAGS, LIBS.
cc="${1:-cc}"
cflags="$2"
libs="${3:--ldwarfp -ldwarf}"
# cflags/libs are intentionally word-split: they carry multiple flags.
# shellcheck disable=SC2086
printf '\043include <libdwarf.h>\n\043include <libdwarfp.h>\nint main(void){Dwarf_P_Debug d=0;Dwarf_Error e=0;return dwarf_producer_init(0,0,0,0,0,"x86_64","V4",0,&d,&e);}\n' |
    $cc $cflags -x c - -o /dev/null $libs >/dev/null 2>&1 && echo 1 || echo 0
