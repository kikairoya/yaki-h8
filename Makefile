HOST=mingw32
#HOST=mingw64
#HOST=linux32
#HOST=linux64
TARGET=h8300hn

DEF_COMMON_CFLAGS=-O2 -g -Wall -Wextra -pedantic
DEF_COMMON_CXXFLAGS=-std=gnu++0x
DEF_COMMON_LINKFLAGS=-Wl,--defsym,binary_stub_tiny_bin_start=_binary_stub_tiny_bin_start -Wl,--defsym,binary_stub_tiny_bin_end=_binary_stub_tiny_bin_end
DEF_CFLAGS_FOR_TARGET=-Os -g -fomit-frame-pointer -Wall -Wextra

ifeq ($(TARGET),h8300hn)
PREFIX_FOR_TARGET=h8300-elf-
CC_FOR_TARGET=$(PREFIX_FOR_TARGET)gcc -mh -mn
CXX_FOR_TARGET=$(PREFIX_FOR_TARGET)g++ -mh -mn
LINK_FOR_TARGET=$(PREFIX_FOR_TARGET)gcc -mh -mn
DEF_LINKFLAGS_FOR_TARGET=$(DEF_CFLAGS_FOR_TARGET) -nostartfiles -nodefaultlibs -nostdlib
OBJCOPY_FOR_TARGET=$(PREFIX_FOR_TARGET)objcopy
endif
ifeq ($(HOST),mingw32)
BUILD=i686-pc-mingw32-
DEF_CFLAGS=-m32
DEF_LINKFLAGS=-m32 -static -Wl,--enable-auto-import
OBJCOPY=$(BUILD)objcopy -B i386 -O pe-i386
EXEEXT=.exe
endif
ifeq ($(HOST),mingw64)
BUILD=x86_64-w64-mingw32-
DEF_CFLAGS=-m64
DEF_LINKFLAGS=-m64 -static -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup
OBJCOPY=$(BUILD)objcopy -B i386:x86-64 -O pe-x86-64
EXEEXT=.exe
endif
ifeq ($(HOST),linux32)
BUILD=i686-redhat-linux-
DEF_CFLAGS=-m32
DEF_LINKFLAGS=-m32
OBJCOPY=$(BUILD)objcopy -B i386 -O elf32-i386
endif
ifeq ($(HOST),linux64)
BUILD=x86_64-redhat-linux-
DEF_CFLAGS=-m64
DEF_LINKFLAGS=-m64
OBJCOPY=$(BUILD)objcopy -B i386:x86-64 -O elf64-x86-64
endif
CC=$(BUILD)gcc
CXX=$(BUILD)g++
LINK=$(BUILD)g++

.PHONY: all stub yaki-h8 clean

all: yaki-h8$(EXEEXT)
stub: stub-tiny.bin

stub-tiny.bin: stub-tiny.elf
	$(OBJCOPY_FOR_TARGET) -Obinary $< $@
stub-tiny.elf: stub-tiny.x stub-tiny.o
	$(LINK_FOR_TARGET) -Tstub-tiny.x stub-tiny.o -o$@ $(DEF_LINKFLAGS_FOR_TARGET) $(LINKFLAGS_FOR_TARGET)
stub-tiny.o: stub-tiny.cc
	$(CXX_FOR_TARGET) -c $< -o$@ $(DEF_CFLAGS_FOR_TARGET) $(DEF_CXXFLAGS_FOR_TARGET) $(CFLAGS_FOR_TARGET) $(CXXFLAGS_FOR_TARGET) 

yaki-h8$(EXEEXT): yaki-h8.o stub-tiny-bin.o
	$(LINK) $^ -o$@ $(DEF_COMMON_LINKFLAGS) $(DEF_LINKFLAGS) $(LINKFLAGS)
yaki-h8.o: yaki-h8.cc
	$(CXX) -c yaki-h8.cc -o$@ $(DEF_COMMON_CFLAGS) $(DEF_CFLAGS) $(CFLAGS) $(DEF_COMMON_CXXFLAGS) $(DEF_CXXFLAGS) $(CXXFLAGS)
stub-tiny-bin.o: stub-tiny.bin
	$(OBJCOPY) -I binary $< $@

clean:
	rm -f stub-tiny.bin stub-tiny.elf stub-tiny.o stub-tiny-bin.o yaki-h8$(EXEEXT) yaki-h8.o
