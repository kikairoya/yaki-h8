HOST=mingw32
#HOST=mingw64
#HOST=linux32
#HOST=linux64
TARGET=h8300hn

DEF_COMMON_CFLAGS= -g -Wall -Wextra -pedantic
DEF_COMMON_CXXFLAGS=-std=gnu++0x
DEF_COMMON_LINKFLAGS=
DEF_ASFLAGS_FOR_TARGET=-g -Wall
DEF_CFLAGS_FOR_TARGET=-Os -g -fomit-frame-pointer -Wall -Wextra

ifeq ($(TARGET),h8300hn)
PREFIX_FOR_TARGET=h8300-elf-
AS_FOR_TARGET=$(PREFIX_FOR_TARGET)gcc
CC_FOR_TARGET=$(PREFIX_FOR_TARGET)gcc
LINK_FOR_TARGET=$(PREFIX_FOR_TARGET)gcc
OBJCOPY_FOR_TARGET=$(PREFIX_FOR_TARGET)objcopy
endif
ifeq ($(HOST),cygwin)
BUILD=i686-pc-cygwin-
DEF_CFLAGS=-m32
DEF_LINKFLAGS=-m32 -static -Wl,--enable-auto-import -g
OBJCOPY=$(BUILD)objcopy -B i386 -O pe-i386
EXEEXT=.exe
endif
ifeq ($(HOST),mingw32)
BUILD=i686-pc-mingw32-
DEF_CFLAGS=-m32
DEF_LINKFLAGS=-m32 -static -Wl,--enable-auto-import -g
OBJCOPY=$(BUILD)objcopy -B i386 -O pe-i386
EXEEXT=.exe
endif
ifeq ($(HOST),mingw64)
BUILD=x86_64-w64-mingw32-
DEF_CFLAGS=-m64
DEF_LINKFLAGS=-m64 -static -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup -g
OBJCOPY=$(BUILD)objcopy -B i386:x86-64 -O pe-x86-64
EXEEXT=.exe
endif
ifeq ($(HOST),linux32)
BUILD=i686-redhat-linux-
DEF_CFLAGS=-m32
DEF_LINKFLAGS=-m32
OBJCOPY=$(BUILD)objcopy -B i386 -O elf32-i386 -g
endif
ifeq ($(HOST),linux64)
BUILD=x86_64-redhat-linux-
DEF_CFLAGS=-m64
DEF_LINKFLAGS=-m64
OBJCOPY=$(BUILD)objcopy -B i386:x86-64 -O elf64-x86-64 -g
endif
CC=$(BUILD)gcc
CXX=$(BUILD)g++
LINK=$(BUILD)g++
CXX_FOR_BUILD=g++

.PHONY: all stub yaki-h8 clean

all: stubs yaki-h8$(EXEEXT)
stubs: stub-h8tiny.dat

bintoc$(EXEEXT): bintoc.cc
	$(CXX_FOR_BUILD) $^ -o $@

stub-%.dat: bintoc$(EXEEXT) stub-%.bin
	./bintoc$(EXEEXT) $(basename $@).bin $@ $(subst -,_,$(basename $@))

stub-%.bin: stub-%.elf
	$(OBJCOPY_FOR_TARGET) -Obinary $< $@

stub-%.o: stub-%.bin
	$(OBJCOPY) -Ibinary $< $@

stub-h8tiny.elf: stub-h8generic.S
	$(CC_FOR_TARGET) $^ -o$@ -Wa,--defsym,_ram=0xFB80 -mh -mn -mrelax -nostartfiles -nodefaultlibs -nostdlib
stub-h8s.elf: stub-h8generic.S
	$(CC_FOR_TARGET) $^ -o$@ -Wa,--defsym,_ram=0xFFFFFB80 -ms -mrelax -nostartfiles -nodefaultlibs -nostdlib
stub-h8stiny.elf: stub-h8generic.S
	$(CC_FOR_TARGET) $^ -o$@ -Wa,--defsym,_ram=0xFB80 -ms -mn -mrelax -nostartfiles -nodefaultlibs -nostdlib

yaki-h8$(EXEEXT): yaki-h8.o
	$(LINK) $^ -o$@ $(DEF_COMMON_LINKFLAGS) $(DEF_LINKFLAGS) $(LINKFLAGS) -lboost_system -lboost_date_time
%.o: %.cc
	$(CXX) -c $^ -o$@ $(DEF_COMMON_CFLAGS) $(DEF_CFLAGS) $(CFLAGS) $(DEF_COMMON_CXXFLAGS) $(DEF_CXXFLAGS) $(CXXFLAGS)

clean:
	rm -f stub-h8 h8tiny.bin stub-h8tiny.elf stub-h8tiny.o stub-h8tiny.dat yaki-h8$(EXEEXT) yaki-h8.o serial.o bintoc$(EXEEXT)
