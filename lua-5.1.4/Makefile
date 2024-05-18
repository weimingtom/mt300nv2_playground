# PLAT := arm-linux-androideabi-
PLAT := STAGING_DIR=/home/wmt/work_mt300nv2/openwrt-toolchain-23.05.3-ramips-mt76x8_gcc-12.3.0_musl.Linux-x86_64/toolchain-mipsel_24kc_gcc-12.3.0_musl /home/wmt/work_mt300nv2/openwrt-toolchain-23.05.3-ramips-mt76x8_gcc-12.3.0_musl.Linux-x86_64/toolchain-mipsel_24kc_gcc-12.3.0_musl/bin/mipsel-openwrt-linux-

CC := $(PLAT)gcc 
CPP := $(PLAT)g++
LD := $(PLAT)ld
AR := $(PLAT)ar rcu 
RANLIB := $(PLAT)ranlib
STRIP := $(PLAT)strip
RM := rm -f

CFLAGS := 
#CFLAGS += -g -O2
CFLAGS += -g3 -O0
CFLAGS += 
CFLAGS += -I.

objects :=
objects += lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o
objects += lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o
objects += lundump.o lvm.o lzio.o lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o
objects += lstrlib.o loadlib.o linit.o

TARGET := lua514

all : $(TARGET)

$(TARGET) : liblua.a
	$(CC) $(CFLAGS) -o $@ lua.c liblua.a -lm

#$(STRIP) $@

liblua.a : $(objects)
	$(AR) $@ $(objects) 
	$(RANLIB) $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(objects) *.a *.exe $(TARGET)

scp:
	scp ./$(TARGET) root@192.168.8.1:/dev/shm/

#scp ./$(TARGET) root@192.168.8.1:/tmp

ssh:
	ssh root@192.168.8.1

