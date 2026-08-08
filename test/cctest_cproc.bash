set -u

# shellcheck disable=SC2034  # referenced indirectly via `local -n` in match_skip
skip_files=(
  # unimplemented
  builtin-inff
  builtin-nanf
  # rcc: automatic/VLA storage over-alignment beyond the ABI-guaranteed 16
  # bytes needs dynamic stack realignment (a distinct RSP-relative base for
  # locals plus preserved RBP-relative incoming-argument addressing),
  # unimplemented - see test/third_party/TODO.md.
  alignas-local-strict
  alignas-vla-strict
)

fix_up() {
  for src in varargs*.c; do
    sed -i 's|__builtin_va_start(ap)|__builtin_c23_va_start(ap)|g' "$src"
  done

  sed -i 's|C4 = C4|C4_ = C4|g' enum-large-value.c

  # overflow'd unicode seq
  sed -i '/char/d' initializer-string-wide.c
  sed -i '/short/d' initializer-string-wide.c
}

match_skip() {
  local -n arr=$2
  for f in "${arr[@]}"; do
    if [ "$f" == "$1" ]; then
      return 0
    fi
  done
  return 1
}

compile_test() {
  for src in *.c; do
    case "$src" in
      preprocess-* | *aarch64* | *riscv64*) continue ;;
    esac
    src="${src%.c}"
    if match_skip "$src" skip_files; then continue; fi

    echo "$src"

    if [ -f "$src.err" ]; then
      if $CC "$src.c" -std=c23 -S -o /dev/null; then exit 1; fi
      continue
    fi

    if ! $CC "$src.c" -std=c23 -c -o ./_tst.o; then exit 1; fi

    if ! $CC ./_tst.o -o ./_tst.exe 2>/dev/null; then continue; fi

    if ! ./_tst.exe; then exit 1; fi
  done
}

cd test || exit 1

fix_up
compile_test
