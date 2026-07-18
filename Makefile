API ?= 35
NDK_ROOT ?= /opt/android-ndk
CC := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$(API)-clang
OUT := build/disabler

CFLAGS := -O2 -g0 -Wall -Wextra -D_GNU_SOURCE -D__ARM \
	-ffunction-sections -fdata-sections -Isrc
LDFLAGS := -static -pthread -Wl,--gc-sections -Wl,--strip-all

SOURCES := \
	src/main.c src/util.c src/page.c src/late_refs.c
HEADERS := src/common.h src/target.h src/kernelsnitch/*.h

.DEFAULT_GOAL := disabler
.PHONY: all disabler clean

all: disabler
disabler: $(OUT)

build:
	mkdir -p $@

$(OUT): $(SOURCES) $(HEADERS) | build
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)
	sha256sum $@
clean:
	rm -rf build
