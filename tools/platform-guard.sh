#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Emit a '#ifndef X / #error / #endif' guard for a platform-specific
# generated header, so a header generated on the wrong host (e.g. copied
# from a stale build tree) fails to compile loudly instead of silently
# feeding the wrong platform's data into the build.
# Usage: platform-guard.sh <header-name> <CC> [predefined-macros-file]
# When the third argument is omitted, probes "<CC> -dM -E -" itself.

header="$1"
cc="$2"
dm="$3"

if [ -z "$dm" ]; then
    dm=$(mktemp)
    trap 'rm -f "$dm"' EXIT
    "$cc" -dM -E - < /dev/null > "$dm"
fi

emit() {
    printf '#ifndef %s\n#error "%s generated for %s"\n#endif\n' "$1" "$header" "$2"
}

if grep -q '__APPLE__' "$dm"; then emit __APPLE__ Apple
elif grep -q '_WIN32' "$dm"; then emit _WIN32 Windows
elif grep -q '__linux__' "$dm"; then emit __linux__ Linux
elif grep -q '__FreeBSD__' "$dm"; then emit __FreeBSD__ FreeBSD
elif grep -q '__OpenBSD__' "$dm"; then emit __OpenBSD__ OpenBSD
elif grep -q '__NetBSD__' "$dm"; then emit __NetBSD__ NetBSD
elif grep -q '__DragonFly__' "$dm"; then emit __DragonFly__ "DragonFly BSD"
fi
