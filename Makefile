# SPDX-License-Identifier: LGPL-2.1-or-later
CC     := gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -g -Isrc
GPERF  := gperf
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

SRCS = src/main.c src/lexer.c src/preprocess.c src/parser.c src/type.c src/codegen.c src/opt.c src/alloc.c src/unicode.c src/keywords.c src/obj.c src/asm.c
OBJS = $(SRCS:.c=$(OBJ_EXT))

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
INCDIR = $(PREFIX)/include/rcc
LIBDIR = $(PREFIX)/lib/rcc
DOCDIR = $(PREFIX)/share/doc/rcc
TARGET_DEPS = $(OBJS) src/rcc.h
TARGET_EXT = $(OBJS)
RUN_TESTS = run_tests

DEF_INCDIR = -DRCC_INCDIR='"$(RCC_INCDIR)"'
VERSION ?= $(shell git describe --long --tags --always 2>/dev/null || echo "v1.2-dev")
MACHINE ?= $(shell $(CC) -dumpmachine 2>/dev/null || echo "unknown")

ifneq ($(findstring apple,$(MACHINE)),)
SRCS += src/macho_write.c src/arm64_enc.c
DARWIN_O = lib/darwin.dylib
TARGET_DEPS += $(DARWIN_O)
else ifneq ($(findstring mingw,$(MACHINE)),)
SRCS += src/coff_write.c src/x86_enc.c
OBJS += $(MINGW_O) -lpthread
TARGET_DEPS = $(OBJS) $(wildcard src/*.h)
else
SRCS += src/elf_write.c src/x86_enc.c
TARGET_DEPS += $(MINGW_O)
TARGET_EXT += $(MINGW_O)
endif

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
EXE_EXT = .exe
SHARED_EXT = .dll
RCC_LIB_LDFLAGS = -shared -Wl,--export-all-symbols -Wl,--enable-auto-import
PREFIX ?= C:/Program Files/rcc
BINDIR = $(PREFIX)
INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib
DOCDIR = $(PREFIX)/doc
SRCS += src/x86_enc.c
else ifneq ($(findstring mingw,$(MACHINE)),)
TARGET = rcc.exe
RUN_TESTS = run_tests.exe
MINGW_O = lib/rcc_mingw$(OBJ_EXT)
TARGET_EXT += -lpthread
OBJ_EXT = .obj
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
ifneq ($(ARM64_SYSROOT),/)
ifeq ($(shell test -d "$(ARM64_SYSROOT)/usr/include" && echo yes),)
ARM64_SYSROOT := /usr/aarch64-redhat-linux/sys-root/fc43
endif
CFLAGS += --sysroot=$(ARM64_SYSROOT)
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
DARWIN_O = lib/rcc_darwin.dylib
OBJS += $(DARWIN_O)
TARGET_DEPS += $(OBJS) $(wildcard src/*.h)
TARGET_DEPS += $(DARWIN_O)
else ifneq ($(findstring mingw,$(MACHINE)),)
TARGET_DEPS += $(OBJS) $(MINGW_O) $(wildcard src/*.h)
OBJS += $(MINGW_O) -lpthread
else
OBJS += $(MINGW_O)
TARGET_DEPS += $(OBJS) $(wildcard src/*.h)
endif
RCC_LIB = rcc_lib$(SHARED_EXT)

ifeq ($(CC),x86_64-w64-mingw32-gcc)
RCC_ALL = $(RCC_LIB)
endif
all: $(TARGET) $(RUN_TESTS) $(RCC_ALL)

$(TARGET): $(TARGET_DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TARGET_EXT)
$(RCC_LIB): $(OBJS) src/lib$(OBJ_EXT) $(MINGW_O)
	$(CC) $(RCC_LIB_LDFLAGS) $(LDFLAGS) -o $@ $(OBJS) src/lib$(OBJ_EXT) $(MINGW_O)

src/keywords.h: src/keywords.gperf src/keyword_ids.h
	$(GPERF) -m 10 --output-file=$@.tmp src/keywords.gperf
	sed -e's,unsigned int hval = len;,unsigned int hval = len \& UINT_MAX;,' \
	    -e's,unsigned int len,size_t len,;' -e 's,register ,,g' <$@.tmp >$@
	rm -f $@.tmp

src/keywords$(OBJ_EXT): src/keywords.c src/keywords.h src/keyword_ids.h

src/sysinc_paths.h: FORCE
	@tmp=$$(mktemp); out=$$(mktemp); plat=$$(mktemp); \
	$(CC) -dM -E - < /dev/null > $$plat; \
	if grep -q '__APPLE__' $$plat; then \
		echo '#ifndef __APPLE__' > $$out; \
		echo '#error "sysinc_paths.h generated for Apple"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '_WIN32' $$plat; then \
		echo '#ifndef _WIN32' > $$out; \
		echo '#error "sysinc_paths.h generated for Windows"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__linux__' $$plat; then \
		echo '#ifndef __linux__' > $$out; \
		echo '#error "sysinc_paths.h generated for Linux"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__FreeBSD__' $$plat; then \
		echo '#ifndef __FreeBSD__' > $$out; \
		echo '#error "sysinc_paths.h generated for FreeBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__OpenBSD__' $$plat; then \
		echo '#ifndef __OpenBSD__' > $$out; \
		echo '#error "sysinc_paths.h generated for OpenBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__NetBSD__' $$plat; then \
		echo '#ifndef __NetBSD__' > $$out; \
		echo '#error "sysinc_paths.h generated for NetBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__DragonFly__' $$plat; then \
		echo '#ifndef __DragonFly__' > $$out; \
		echo '#error "sysinc_paths.h generated for DragonFly BSD"' >> $$out; \
		echo '#endif' >> $$out; \
	fi; \
	RCC_CC="$(CC)"; \
	if [ "$(CC)" = "aarch64-linux-gnu-gcc" ] || [ -n "$(ARM64_SYSROOT)" ]; then \
		./tools/get-sysinc-paths.sh "$(CC) --sysroot=$(ARM64_SYSROOT)" >> $$out; \
	else \
		./tools/get-sysinc-paths.sh $(CC) >> $$out; \
	fi; \
	rm -f $$plat; \
	if [ -f $@ ] && cmp -s $$out $@; then rm -f $$out; else mv $$out $@; fi

src/gcc_predefined.h: FORCE
	@tmp=$$(mktemp); out=$$(mktemp); \
	$(CC) -dM -E - < /dev/null > $$tmp; \
	if grep -q '__APPLE__' $$tmp; then \
		echo '#ifndef __APPLE__' > $$out; \
		echo '#error "gcc_predefined.h generated for Apple"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '_WIN32' $$tmp; then \
		echo '#ifndef _WIN32' > $$out; \
		echo '#error "gcc_predefined.h generated for Windows"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__linux__' $$tmp; then \
		echo '#ifndef __linux__' > $$out; \
		echo '#error "gcc_predefined.h generated for Linux"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__FreeBSD__' $$tmp; then \
		echo '#ifndef __FreeBSD__' > $$out; \
		echo '#error "gcc_predefined.h generated for FreeBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__OpenBSD__' $$tmp; then \
		echo '#ifndef __OpenBSD__' > $$out; \
		echo '#error "gcc_predefined.h generated for OpenBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__NetBSD__' $$tmp; then \
		echo '#ifndef __NetBSD__' > $$out; \
		echo '#error "gcc_predefined.h generated for NetBSD"' >> $$out; \
		echo '#endif' >> $$out; \
	elif grep -q '__DragonFly__' $$tmp; then \
		echo '#ifndef __DragonFly__' > $$out; \
		echo '#error "gcc_predefined.h generated for DragonFly BSD"' >> $$out; \
		echo '#endif' >> $$out; \
	else \
		: > $$out; \
	fi; \
	awk -f tools/get-gcc-predefined.awk $$tmp >> $$out; \
	rm -f $$tmp; \
	if [ -f $@ ] && cmp -s $$out $@; then rm -f $$out; else mv $$out $@; fi

$(DARWIN_O): lib/rcc_darwin.c
	$(CC) -arch arm64 -dynamiclib -install_name @rpath/rcc_darwin.dylib -o $@ lib/rcc_darwin.c
$(MINGW_O): lib/rcc_mingw.c
	$(CC) $(filter-out -flto=auto -flto=thin,$(CFLAGS)) -c lib/rcc_mingw.c -o $@
src/main$(OBJ_EXT): src/main.c src/sysinc_paths.h
	$(CC) $(CFLAGS) -c src/main.c -o $@ -DGCC=\"$(RCC_GCC)\" $(DEF_INCDIR) -DVERSION=\"$(VERSION)\" -DMACHINE=\"$(MACHINE)\"
src/preprocess$(OBJ_EXT): src/preprocess.c src/sysinc_paths.h src/gcc_predefined.h
	$(CC) $(CFLAGS) -c src/preprocess.c -o $@ $(DEF_INCDIR)
src/unicode$(OBJ_EXT): src/unicode.c src/unicode.h
	$(CC) $(CFLAGS) -c src/unicode.c -o $@
src/lib$(OBJ_EXT): src/lib.c src/rcc.h src/rcc_lib.h
	$(CC) $(CFLAGS) -c src/lib.c -o $@

run_tests: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c
run_tests.exe: run_tests.c
	$(CC) $(CFLAGS) -o $@ run_tests.c
run_tests_arm64: run_tests.c
	@sysroot="$$(aarch64-linux-gnu-gcc -print-sysroot 2>/dev/null)"; \
	if [ -z "$$sysroot" ] || [ "$$sysroot" = "/" ] || [ ! -f "$$sysroot/usr/include/stdio.h" ]; then \
	    for p in /usr/aarch64-redhat-linux/sys-root/fc43 /usr/aarch64-redhat-linux/sys-root/fc44 /usr/aarch64-linux-gnu/sys-root; do \
	        if [ -f "$$p/usr/include/stdio.h" ]; then sysroot="$$p"; break; fi; \
	    done; \
	fi; \
	aarch64-linux-gnu-gcc -std=c11 -Wall -Wextra -O2 -Isrc \
	  --sysroot="$$sysroot" -o $@ run_tests.c

%$(OBJ_EXT): %.c
	$(CC) $(CFLAGS) -c $< -o $@

compile_commands.json: $(SRCS)
	$(MAKE) clean
	bear -- make

# Profile build: rcc compiled with -pg for gprof analysis
rcc_prof: CFLAGS += -pg
rcc_prof: $(SRCS) src/rcc.h src/sysinc_paths.h src/gcc_predefined.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) -DGCC=\"$(RCC_GCC)\" $(DEF_INCDIR) -DVERSION=\"$(VERSION)\" -DMACHINE=\"$(MACHINE)\" -lm

# Run profile: compile a decent-sized file to generate gmon.out
prof: rcc_prof
	./rcc_prof -E bench/bench.c > /dev/null
	gprof ./rcc_prof gmon.out > gprof.txt
	@echo "Profile written to gprof.txt"
	@head -40 gprof.txt

ifeq ($(OS),Windows_NT)
TEST_RUNNER = ./run_tests ./rcc.exe --parallel
BENCH_RUNNER = powershell -ExecutionPolicy Bypass -File bench/run_bench.ps1 ./$(TARGET)
else
TEST_RUNNER = ./run_tests ./rcc --parallel
BENCH_RUNNER = ./bench/run_bench.sh ./$(TARGET)
endif
test check: $(TARGET) $(RUN_TESTS)
	$(TEST_RUNNER)
test-all check-all: $(TARGET) $(RUN_TESTS) lint
	./$(RUN_TESTS) ./$(TARGET) --all --parallel
test-torture check-torture: $(TARGET) $(RUN_TESTS)
	./$(RUN_TESTS) ./$(TARGET) --torture --parallel
test-full check-full:
	$(MAKE) clean
	$(MAKE) check-all
	-./mingw-test.sh
	-./arm64-test.sh
	-./darwin-test.sh

lint:
	if command -v prek; then prek run -a; \
        elif command -v pre-commit; then pre-commit run --all-files; fi

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

# Rebuild with the installed include path so rcc finds its headers
# without needing -I after installation.
install: $(TARGET)
	$(MAKE) clean
	$(MAKE) RCC_INCDIR="$(INCDIR)"
ifeq ($(OS),Windows_NT)
	install -d "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(BINDIR)),$(BINDIR))" "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(INCDIR)),$(INCDIR))" "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(DOCDIR)),$(DOCDIR))"
	install -m 755 $(TARGET) "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(BINDIR)),$(BINDIR))/"
	install -m 644 include/* "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(INCDIR)),$(INCDIR))/"
	install -m 644 README.md test/tcc_test*.md test_report*.md LICENSE bench/bench_report*.md "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(DOCDIR)),$(DOCDIR))/"
	if test -n "$(MINGW_O)"; then install -d "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))"; install -m 644 $(MINGW_O) "$(if $(DESTDIR),$(DESTDIR)$(subst C:,,$(LIBDIR)),$(LIBDIR))/"; fi
else
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(INCDIR)" "$(DESTDIR)$(DOCDIR)"
	install -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/"
	install -m 644 include/* "$(DESTDIR)$(INCDIR)/"
	install -m 644 README.md test/tcc_test*.md test_report*.md LICENSE bench/bench_report*.md "$(DESTDIR)$(DOCDIR)/"
	if test -n "$(MINGW_O)"; then install -d "$(DESTDIR)$(LIBDIR)"; install -m 644 $(MINGW_O) "$(DESTDIR)$(LIBDIR)/"; fi
endif

dist: $(TARGET)
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
	rm -f src/sysinc_paths.h src/gcc_predefined.h fred.txt qemu*.core
	if command -v git > /dev/null 2>&1; then \
	  cd tinycc && git reset --hard && git clean -dxf tests/tests2 && cd ..; \
	  cd c-testsuite && git clean -dxf . && cd ..; \
	fi
clean:
	rm -f $(OBJS) $(TARGET) $(RUN_TESTS) $(RCC_LIB) rcc_prof \
	      src/sysinc_paths.h src/gcc_predefined.h src/keywords.h.tmp \
	      fred.txt *.s qemu*.core src/*.obj src/*.darwin.o src/*.arm64.o \
	      lib/rcc_mingw$(OBJ_EXT) lib/rcc_darwin$(OBJ_EXT) test-tcc-*.summary test-ctest-*.summary test-compliance-*.summary
	if command -v git > /dev/null 2>&1; then \
	  cd tinycc && git reset --hard && git clean -dxf tests/tests2 && cd ..; \
	  cd c-testsuite && git clean -dxf . && cd ..; \
	fi

TAGS: $(SRCS) src/rcc.h
	etags -a --language=c src/*.c src/*.h

.PHONY: clean leanclean test check test-extra check-extra check-full check-torture \
	test-full test-torture lint bench install dist bench prof FORCE
FORCE:
