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

objects := tinybasic.o

TARGET := tinybasic

all : $(TARGET)

$(TARGET) : $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) -lm

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CPP) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(objects) *.a *.exe *.o $(TARGET)
	
scp:
	scp ./$(TARGET) root@192.168.8.1:/dev/shm/
	scp ./*.BAS root@192.168.8.1:/dev/shm/
	
#scp ./$(TARGET) root@192.168.8.1:/tmp

ssh:
	ssh root@192.168.8.1

