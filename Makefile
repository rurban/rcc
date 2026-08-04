# SPDX-License-Identifier: LGPL-2.1-or-later
CC     := gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -g -Isrc
GPERF  := gperf
POD2MAN := pod2man
TARGET = rcc
MINGW_O =
OBJ_EXT = .o
EXE_EXT =
SHARED_EXT = .so
RCC_LIB_LDFLAGS = -shared -fPIC
# Backend C compiler invoked by the generated rcc binary itself (assembler/
# linker step). Defaults to $(CC), but the mingw cross build produces
# rcc.exe which runs on Windows, where the toolchain is normally just "gcc"
# (e.g. MSYS2/MinGW-w64), not "x86_64-w64-mingw32-gcc".
RCC_GCC = $(CC)

ifeq ($(ASAN),1)
CFLAGS = -std=c11 -Wall -Wextra -g -Isrc
CFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address
endif

# Detect clang vs gcc
IS_CLANG := $(shell $(CC) --version 2>/dev/null | grep -c clang)

ifeq ($(IS_CLANG),0)
CFLAGS += -flto=auto
else
CFLAGS += -flto=thin
# Probe for LLVM LTO plugin (needed when linking with ld.bfd/ld.gold).
# Try llvm-config-<major> first (avoids broken alternatives symlinks), then llvm-config.
#LLVM_LIBDIR := $(shell \
#    major=$$($(CC) --version 2>/dev/null | grep -oE 'clang version [0-9]+' | grep -oE '[0-9]+'); \
#    for cmd in "llvm-config-$$major" llvm-config; do \
#        d=$$($$cmd --libdir 2>/dev/null) && test -n "$$d" && echo "$$d" && break; \
#    done)
#ifneq ($(LLVM_LIBDIR),)
#LTO_PLUGIN := $(LLVM_LIBDIR)/LLVMgold.so
#ifneq ($(wildcard $(LTO_PLUGIN)),)
#LDFLAGS += -Wl,-plugin,$(LTO_PLUGIN)
#endif
#endif
endif

SRCS = src/main.c src/lexer.c src/preprocess.c src/parser.c src/type.c src/codegen.c src/cg_builtins.c src/cg_vectors.c src/opt.c src/alloc.c src/unicode.c src/keywords.c src/obj.c src/asm.c
# Shared headers every object must be rebuilt for (see %$(OBJ_EXT) rule).
HDRS = $(wildcard src/*.h)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
INCDIR = $(PREFIX)/include/rcc
LIBDIR = $(PREFIX)/lib/rcc
DOCDIR = $(PREFIX)/share/doc/rcc
MANDIR = $(PREFIX)/share/man/man1
SRCS = src/main.c src/lexer.c src/preprocess.c src/parser.c src/type.c src/codegen.c src/cg_builtins.c src/cg_vectors.c src/opt.c src/alloc.c src/unicode.c src/keywords.c src/obj.c src/asm.c src/link.c
TARGET_EXT = $(OBJS)
RUN_TESTS = run_tests

DEF_INCDIR = -DRCC_INCDIR='"$(RCC_INCDIR)"'
VERSION ?= $(shell git describe --long --tags --always 2>/dev/null || echo "v1.2-dev")
MACHINE ?= $(shell $(CC) -dumpmachine 2>/dev/null || echo "unknown")

ifneq ($(findstring apple,$(MACHINE)),)
SRCS += src/macho_write.c src/link_macho.c src/arm64_enc.c
else ifneq ($(findstring mingw,$(MACHINE)),)
SRCS += src/coff_write.c src/link_pe.c src/x86_enc.c
else
SRCS += src/elf_write.c src/link_elf.c src/x86_enc.c
TARGET_DEPS += $(MINGW_O)
TARGET_EXT += $(MINGW_O)
endif
OBJS = $(SRCS:.c=$(OBJ_EXT))
TARGET_DEPS = $(OBJS) $(wildcard src/*.h)
LIBDFP_A = lib/libdfp.a

# Build-time include directory: absolute path to the source include/ dir.
# Override this when installing to a different prefix.
RCC_INCDIR ?= $(CURDIR)/include

# On native Windows builds, default to the standard install location.
ifeq ($(OS),Windows_NT)
TARGET = rcc.exe
RUN_TESTS = run_tests.exe
MINGW_O = lib/rcc_mingw$(OBJ_EXT)
TARGET_EXT += -lpthread
OBJ_EXT = .obj
# LTO+-O3 miscompiles rcc.exe itself for this target: building c23-
# complit-4.c's `(static thread_local int[]){1,2}` crashed rcc.exe with a
# SIGSEGV inside/around cg_emit_emutls_data() (confirmed via wine +
# addr2line — the crash disappeared with an otherwise-identical -flto-less
# rebuild, and a debug dump showed the LVar's own fields, e.g. init_size,
# were correct going in). rcc_mingw.c already opts out of LTO for the same
# reason (see $(MINGW_O) rule below); apply it to the whole target instead
# of chasing one GCC/LTO optimization-pass interaction.
CFLAGS := $(filter-out -flto=auto -flto=thin,$(CFLAGS))
EXE_EXT = .exe
SHARED_EXT = .dll
RCC_LIB_LDFLAGS = -shared -Wl,--export-all-symbols -Wl,--enable-auto-import
PREFIX ?= C:/Program Files/rcc
BINDIR = $(PREFIX)
INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib
DOCDIR = $(PREFIX)/doc
LIBDFP_A = lib/libdfp.lib
else ifneq ($(findstring mingw,$(MACHINE)),)
TARGET = rcc.exe
RUN_TESTS = run_tests.exe
MINGW_O = lib/rcc_mingw$(OBJ_EXT)
TARGET_EXT += -lpthread
OBJ_EXT = .obj
LIBDFP_A = lib/libdfp.lib
# See the native-Windows block above for why LTO is excluded here too.
CFLAGS := $(filter-out -flto=auto -flto=thin,$(CFLAGS))
EXE_EXT = .exe
SHARED_EXT = .dll
RCC_LIB_LDFLAGS = -shared -Wl,--export-all-symbols -Wl,--enable-auto-import
OBJS = $(SRCS:.c=$(OBJ_EXT))
# rcc.exe runs on Windows; its backend toolchain is "gcc.exe".
# .exe is needed under Wine (CreateProcess can't run ELF binaries
# mapped through Z:\), and it works equally on native Windows.
RCC_GCC = gcc.exe
endif
ifneq ($(findstring aarch64,$(MACHINE)),)
TARGET = rcc-arm64
RUN_TESTS = run_tests_arm64
SRCS += src/arm64_enc.c
OBJ_EXT = .arm64.o
ARM64_SYSROOT := $(shell $(CC) -print-sysroot 2>/dev/null)
LIBDFP_A = lib/libdfp-arm64.a
# Fedora aarch64 gcc may inject a broken -latomic_asneeded spec.
# Add -fno-link-libatomic only when this compiler needs and supports it.
ARM64_NO_LINK_LIBATOMIC := $(shell $(CC) -dumpspecs 2>/dev/null | grep -q latomic_asneeded && $(CC) -fno-link-libatomic -x c -c /dev/null -o /dev/null >/dev/null 2>&1 && echo yes)
ifeq ($(ARM64_NO_LINK_LIBATOMIC),yes)
CFLAGS += -fno-link-libatomic
endif
ifneq ($(ARM64_SYSROOT),/)
ifeq ($(shell test -d "$(ARM64_SYSROOT)/usr/include" && echo yes),)
# Try fc44, then fc43, then fc41 for older Fedora versions
ARM64_SYSROOT := $(firstword $(foreach v,fc44 fc43 fc41,$(if $(wildcard /usr/aarch64-redhat-linux/sys-root/$v/usr/include),/usr/aarch64-redhat-linux/sys-root/$v)))
endif
CFLAGS += --sysroot=$(ARM64_SYSROOT)
endif
endif
ifneq ($(findstring musl,$(CC)),)
CFLAGS += -D__MUSL__
OBJ_EXT = .musl.o
TARGET = rcc-musl
RUN_TESTS = run_tests_musl
# musl-gcc is a glibc-gcc wrapper (its specs only redirect include/lib
# paths), so `-dumpmachine` reports the host glibc triple. Reflect the
# musl libc in MACHINE instead, so --version/-dumpmachine don't misreport
# a glibc target.
ifeq ($(findstring musl,$(MACHINE)),)
MACHINE := $(patsubst %-linux,%-linux-musl,$(MACHINE))
endif
endif
OBJS = $(SRCS:.c=$(OBJ_EXT))

# Native Linux builds: optimize for the host CPU
ifeq ($(shell uname -s),Linux)
ifeq ($(CC),gcc)
CFLAGS += -march=native
endif
ifneq ($(IS_CLANG),0)
CFLAGS += -march=native
endif
endif
DEF_INCDIR = -DRCC_INCDIR='"$(RCC_INCDIR)"'
VERSION ?= $(shell git describe --long --tags --always 2>/dev/null || echo "v1.2-dev")

ifneq ($(findstring apple,$(MACHINE)),)
# The dyld-interpose runtime shim (on_exit/exit handling) needs a real Apple
# toolchain (-arch/-dynamiclib/-install_name) to build, so only require it
# when actually running on a Darwin host. A Linux host cross-building with
# MACHINE=*apple* (used for compile-only Mach-O codegen testing, since we
# can't execute Mach-O binaries on Linux) must skip it.
ifeq ($(shell uname -s),Darwin)
DARWIN_O = lib/rcc_darwin.dylib
LDFLAGS += -Wl,-rpath,@executable_path/lib
OBJS += $(DARWIN_O)
LIBDFP_A = lib/libdfp.a
else
LIBDFP_A = lib/libdfp-darwin.a
endif
TARGET_DEPS += $(OBJS) $(wildcard src/*.h)
else ifneq ($(findstring mingw,$(MACHINE)),)
TARGET_DEPS += $(OBJS) $(MINGW_O) $(wildcard src/*.h)
OBJS += $(MINGW_O)
LIBDFP_A = lib/libdfp.lib
else
OBJS += $(MINGW_O)
TARGET_DEPS += $(OBJS) $(wildcard src/*.h)
endif
# libdfp references glibc-specific fesetexcept/fegetexcept; skip for musl
ifneq ($(findstring musl,$(CC)),)
LIBDFP_A =
endif

# iconv is optional; -fexec-charset depends on it
HAVE_ICONV := $(shell printf '\#include <iconv.h>\nint main(){}\n' > /tmp/_ic.c; $(CC) /tmp/_ic.c -o /dev/null -liconv 2>/dev/null && echo 1; echo 0; rm -f /tmp/_ic.c)
ifeq ($(HAVE_ICONV),1)
CFLAGS += -DHAVE_ICONV
LDFLAGS += -liconv
endif
RCC_LIB = rcc_lib$(SHARED_EXT)

all: $(TARGET) $(RUN_TESTS) $(RCC_ALL) $(RCC_LIB) $(LIBDFP_A)

# Bundled IEEE 754-2008 decimal (BID) runtime: the pure-C libbid core from
# libdfp 1.0.17 (LGPL-2.1-or-later, same as rcc; see lib/libdfp/COPYING*)
# plus rcc's own __bid_*3/__bid_*2 wrapper layer (lib/libdfp/rcc_dec_rt.c).
# Built with the same CC as the target so host, arm64 cross and mingw cross
# builds each get a matching archive; rcc's native linker auto-links it.
# rcc_dec_rt.c declares no _Decimal types (BID bit-pattern ABI), so it
# compiles even where the C compiler has no decimal support (aarch64 gcc).
LIBDFP_DIR = lib/libdfp
# No TLS: the decimal rounding/exception globals are plain (single-threaded
# semantics are fine for rcc's use), which keeps the archive free of TLS
# relocs rcc's native linker doesn't handle on every target (x86-64 LE is
# mapped, but arm64 gcc emits TLS-DESC (R_AARCH64_TLS_DESC) regardless of
# -ftls-model=local-exec).
# Needs the same --sysroot as CFLAGS on ARM64 cross builds (Fedora's
# aarch64-linux-gnu-gcc has no baked-in default sysroot, unlike e.g.
# Ubuntu's, and $(CC) here is the exact same cross-compiler as CFLAGS
# uses -- extracted rather than duplicating ARM64_SYSROOT's own
# fc44/fc43/fc41 fallback probing).
LIBDFP_CFLAGS = -O2 -fPIC -I$(LIBDFP_DIR) -DBID_HAS_GCC_DECIMAL_INTRINSICS=0 $(filter --sysroot=%,$(CFLAGS))
LIBDFP_SRCS := $(wildcard $(LIBDFP_DIR)/*.c)
LIBDFP_OBJS := $(patsubst $(LIBDFP_DIR)/%.c,build/libdfp$(OBJ_EXT)/%.o,$(LIBDFP_SRCS))

$(LIBDFP_A): $(LIBDFP_OBJS)
	ar rcs $@ $(LIBDFP_OBJS)

build/libdfp$(OBJ_EXT)/%.o: $(LIBDFP_DIR)/%.c
	@mkdir -p build/libdfp$(OBJ_EXT)
	$(CC) $(LIBDFP_CFLAGS) -c $< -o $@

$(TARGET): $(TARGET_DEPS) $(LIBDFP_A)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TARGET_EXT) $(LIBDFP_A) -lm
$(RCC_LIB): $(OBJS) src/lib$(OBJ_EXT) $(MINGW_O) $(LIBDFP_A)
	$(CC) $(CFLAGS) $(RCC_LIB_LDFLAGS) $(LDFLAGS) -o $@ src/lib$(OBJ_EXT) $(TARGET_EXT) $(LIBDFP_A) -lm

src/keywords.h: src/keywords.gperf src/keyword_ids.h
	$(GPERF) -m 10 --output-file=$@.tmp src/keywords.gperf
	sed -e's,unsigned int hval = len;,unsigned int hval = len \& UINT_MAX;,' \
	    -e's,unsigned int len,size_t len,;' -e 's,register ,,g' <$@.tmp >$@
	rm -f $@.tmp

src/keywords$(OBJ_EXT): src/keywords.c src/keywords.h src/keyword_ids.h

src/sysinc_paths.h: FORCE
	@./tools/gen-sysinc-paths.sh "$(CC)" "$(ARM64_SYSROOT)" $@

src/bitint_rt.h: src/bitint_rt.c tools/embed-c.sh
	@./tools/gen-bitint-rt-h.sh $@

src/gcc_predefined.h: FORCE
	@./tools/gen-gcc-predefined.sh "$(CC)" $@

$(DARWIN_O): lib/rcc_darwin.c
	$(CC) -arch arm64 -dynamiclib -install_name $(PWD)/lib/rcc_darwin.dylib -o $@ lib/rcc_darwin.c
$(MINGW_O): lib/rcc_mingw.c
	$(CC) $(filter-out -flto=auto -flto=thin,$(CFLAGS)) -c lib/rcc_mingw.c -o $@
src/codegen$(OBJ_EXT): src/codegen.c src/bitint_rt.h $(HDRS)
	$(CC) $(CFLAGS) -c src/codegen.c -o $@
src/main$(OBJ_EXT): src/main.c src/sysinc_paths.h src/bitint_rt.h $(HDRS)
	$(CC) $(CFLAGS) -c src/main.c -o $@ -DGCC=\"$(RCC_GCC)\" $(DEF_INCDIR) -DVERSION=\"$(VERSION)\" -DMACHINE=\"$(MACHINE)\"
src/preprocess$(OBJ_EXT): src/preprocess.c src/sysinc_paths.h src/gcc_predefined.h $(HDRS)
	$(CC) $(CFLAGS) -c src/preprocess.c -o $@ $(DEF_INCDIR)
src/unicode$(OBJ_EXT): src/unicode.c src/unicode.h
	$(CC) $(CFLAGS) -c src/unicode.c -o $@
src/lib$(OBJ_EXT): src/lib.c src/rcc_lib.h $(HDRS)
	$(CC) $(CFLAGS) -c src/lib.c -o $@

run_tests: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c
run_tests.exe: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c
run_tests_musl: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c
run_tests_arm64: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c

# Every object depends on the shared headers: a stale object compiled
# against an older rcc.h gets a different struct layout than its peers
# (ODR skew), which shows up as arbitrary memory corruption at runtime.
%$(OBJ_EXT): %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

compile_commands.json: $(SRCS)
	$(MAKE) clean
	bear -- make

# Profile build: rcc compiled with -pg for gprof analysis
rcc_prof: $(SRCS) src/rcc.h src/sysinc_paths.h src/gcc_predefined.h
	$(CC) $(CFLAGS) -pg $(LDFLAGS) -o $@ $(SRCS) -DGCC=\"$(RCC_GCC)\" $(DEF_INCDIR) -DVERSION=\"$(VERSION)\" -DMACHINE=\"$(MACHINE)\" -lm

# Run profile: compile a decent-sized file to generate gmon.out
prof: rcc_prof
	./rcc_prof -E bench/bench.c > /dev/null
	gprof ./rcc_prof gmon.out > gprof.txt
	@echo "Profile written to gprof.txt"
	@head -40 gprof.txt

ifeq ($(OS),Windows_NT)
TEST_RUNNER = ./run_tests.exe ./rcc.exe
TEST_RUNNER_O1 = ./run_tests.exe "./rcc.exe -O1"
TEST_RUNNER_O2 = ./run_tests.exe "./rcc.exe -O2"
else
TEST_RUNNER = ./run_tests ./rcc
TEST_RUNNER_O1 = ./run_tests "./rcc -O1"
TEST_RUNNER_O2 = ./run_tests "./rcc -O2"
BENCH_RUNNER = ./bench/run_bench.sh ./$(TARGET)
endif

# Shell-driven link test (shared libs + static archives, incl. sqlite3.c).
# Native builds only: a cross-built rcc.exe/darwin binary can't run here.
ifeq ($(EXE_EXT),)
LINK_TEST = ./test/test-link.sh ./$(TARGET)
else
LINK_TEST = true
endif
test: $(TARGET) $(RUN_TESTS)
	rm -f bash.log; ulimit -f 1048576; $(TEST_RUNNER) --parallel
	$(LINK_TEST)
check: test
test-all check-all: $(TARGET) $(RUN_TESTS) lint-changed
	ulimit -f 2097152; $(TEST_RUNNER) --all --parallel
	$(LINK_TEST)
test-link check-link: $(TARGET)
	$(LINK_TEST)
test-unit check-unit: $(TARGET) $(RUN_TESTS)
	ulimit -f 2097152; $(TEST_RUNNER) --unit-tests --parallel
test-compliance check-compliance: $(TARGET) $(RUN_TESTS)
	ulimit -f 2097152; $(TEST_RUNNER) --compliance --parallel
test-ctest check-ctest: $(TARGET) $(RUN_TESTS)
	ulimit -f 2097152; $(TEST_RUNNER) --ctest --parallel
test-torture check-torture: $(TARGET) $(RUN_TESTS)
	ulimit -f 2097152; $(TEST_RUNNER) --torture --parallel
test-musl check-musl:
	-$(MAKE) CC=musl-gcc && ./run_tests_musl ./rcc-musl --all --parallel
test-gcc-bugs check-gcc-bugs: $(TARGET) $(RUN_TESTS)
	ulimit -f 2097152; $(TEST_RUNNER) --gcc-bugs --parallel
test-full check-full:
	$(MAKE) clean && $(MAKE) check-all && $(MAKE) check-musl
	ulimit -f 2097152; $(TEST_RUNNER_O1) --parallel && $(TEST_RUNNER_O2) --parallel
	./mingw-test.sh || true; ./arm64-test.sh || true; ./darwin-test.sh || true

# External project tests via test/linux_thirdparty.bash, built with rcc.
# List of targets lives in test/third_party/targets.txt (regenerate with:
#   grep -o '^test_[a-zA-Z0-9_]*' test/linux_thirdparty.bash | sort -u)
THIRDPARTY_TARGETS := $(shell cat test/third_party/targets.txt 2>/dev/null)
thirdparty-list:
	@cat test/third_party/targets.txt
test-thirdparty check-thirdparty: $(TARGET)
	./test/third_party/run_batch.sh $(THIRDPARTY_TARGETS)

check-bootstrap:
	make install
	make clean
	make CC=rcc rcc run_tests
	./run_tests ./rcc --all --parallel

lint:
	if command -v prek; then prek run -a; \
        elif command -v pre-commit; then pre-commit run --all-files; fi
	if command -v checkmake; then checkmake Makefile; fi

lint-changed:
	if command -v prek > /dev/null 2>&1; then prek run -s HEAD~1; \
	elif command -v pre-commit > /dev/null 2>&1; then pre-commit run --from-ref HEAD~1 --to-ref HEAD; fi

tcc: tinycc/tcc tinycc/lib/tcc/include

tinycc/tcc: tinycc/config.mak FORCE
	$(MAKE) -C tinycc $(if $(V),,V=) tcc

tinycc/config.mak: tinycc/configure
	cd tinycc && ./configure --prefix=$(CURDIR)/tinycc --tccdir=$(CURDIR)/tinycc/lib/tcc

tinycc/lib/tcc/include:
	mkdir -p tinycc/lib/tcc
	ln -sf ../../include tinycc/lib/tcc/include

bench: $(TARGET)
	$(BENCH_RUNNER)

# rcc.1 man page, generated from docs/rcc.pod (see docs/rcc.md for the
# prose reference both are kept in sync with).
man: docs/rcc.1

docs/rcc.1: docs/rcc.pod
	if command -v $(POD2MAN); then $(POD2MAN) --section=1 --center="RCC C Compiler" --release="rcc $(VERSION)" --name=RCC docs/rcc.pod $@; else touch $@; fi

# Rebuild with the installed include path so rcc finds its headers
# without needing -I after installation.
install: $(TARGET)
	$(MAKE) clean
	$(MAKE) RCC_INCDIR="$(INCDIR)"
ifeq ($(OS),Windows_NT)
	install -d "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(BINDIR)),$(BINDIR))" "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(INCDIR)),$(INCDIR))" "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(DOCDIR)),$(DOCDIR))"
	install -m 755 $(TARGET) "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(BINDIR)),$(BINDIR))/"
	install -m 644 include/* "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(INCDIR)),$(INCDIR))/"
	install -m 644 README.md docs/rcc.md test/tcc_test*.md test_report*.md LICENSE bench/bench_report*.md "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(DOCDIR)),$(DOCDIR))/"
	install -d "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))"
	install -m 755 $(RCC_LIB) "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))/"
	if test -n "$(MINGW_O)"; then install -d "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))"; install -m 644 $(MINGW_O) "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))/"; fi
else
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(INCDIR)" "$(DESTDIR)$(DOCDIR)"
	install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/" || sudo install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/"
	install -m 644 include/* "$(DESTDIR)$(INCDIR)/"
	install -m 644 README.md docs/*.md test/tcc_test*.md test_report*.md LICENSE bench/bench_report*.md "$(DESTDIR)$(DOCDIR)/"
	install -d "$(DESTDIR)$(LIBDIR)"
	install -m 644 $(RCC_LIB) "$(DESTDIR)$(LIBDIR)/"
	@if test -n "$(MINGW_O)"; then install -m 644 $(MINGW_O) "$(DESTDIR)$(LIBDIR)/"; fi
	@if test -n "$(DARWIN_O)"; then install -m 644 $(DARWIN_O) "$(DESTDIR)$(LIBDIR)/"; fi
	@if command -v $(POD2MAN) > /dev/null 2>&1; then \
	  $(MAKE) man; \
	  install -d "$(DESTDIR)$(MANDIR)"; \
	  install -m 644 docs/rcc.1 "$(DESTDIR)$(MANDIR)/"; \
	else \
	  echo "pod2man not found: skipping rcc.1 man page install"; \
	fi
endif

dist: $(TARGET) docs/rcc.1
	if test "$(shell git diff --raw)" != "" || \
           test "$(shell git diff --cached --raw)" != "" ; then \
          echo 'You are not on a clean branch, aborting.'; \
          exit 1; \
	fi
	@echo make dist on $(OS)
	@rm -rf rcc-$(VERSION) || true
ifeq ($(OS),Windows_NT)
	$(MAKE) install DESTDIR="rcc-$(VERSION)" PREFIX=""
	cd rcc-$(VERSION) && powershell -command "Compress-Archive -Path * -DestinationPath ../rcc-$(VERSION).zip -Force"
	rm -rf rcc-$(VERSION)
	git checkout-index --prefix=rcc-$(VERSION)-src/ -a
	cd rcc-$(VERSION)-src && powershell -command "Compress-Archive -Path * -DestinationPath ../rcc-$(VERSION)-src.zip -Force"
else
	$(MAKE) install DESTDIR="rcc-$(VERSION)"
	tar cfz rcc-$(VERSION).tar.gz rcc-$(VERSION)
	tar cfJ rcc-$(VERSION).tar.xz rcc-$(VERSION)
	rm -rf rcc-$(VERSION)
	git checkout-index --prefix=rcc-$(VERSION)-src/ -a
	tar cfz rcc-$(VERSION)-src.tar.gz rcc-$(VERSION)-src
	tar cfJ rcc-$(VERSION)-src.tar.xz rcc-$(VERSION)-src
endif
	rm -rf rcc-$(VERSION)-src

leanclean:
	rm -f src/sysinc_paths.h src/gcc_predefined.h fred.txt qemu*.core test/torture/core.*
	if command -v git > /dev/null 2>&1; then \
	  cd tinycc && git reset --hard && git clean -dxf tests/tests2 && cd ..; \
	  cd c-testsuite && git clean -dxf . && cd ..; \
	fi
clean:
	rm -f $(OBJS) $(TARGET) $(RUN_TESTS) $(RCC_LIB) rcc_prof src/sysinc_paths.h src/gcc_predefined.h src/keywords.h.tmp fred.txt *.s qemu*.core test/torture/core.* docs/rcc.1 src/*.obj src/*.darwin.o src/*.arm64.o src/*.musl.o lib/rcc_mingw$(OBJ_EXT) lib/rcc_darwin$(OBJ_EXT) test-tcc-*.summary test-ctest-*.summary test-compliance-*.summary
	if command -v git > /dev/null 2>&1; then \
	  cd tinycc && git reset --hard && git clean -dxf tests/tests2 && cd ..; \
	  cd c-testsuite && git clean -dxf . && cd ..; \
	fi

TAGS: $(SRCS) src/rcc.h
	etags -a --language=c src/*.c src/*.h

.PHONY: all clean leanclean test check check-full check-torture check-all test-all
.PHONY: test-full test-torture test-unit check-unit test-compliance check-compliance test-ctest check-ctest test-link check-link
.PHONY: test-thirdparty check-thirdparty thirdparty-list
.PHONY: lint lint-changed bench install dist prof man tcc FORCE
FORCE:
