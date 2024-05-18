# PLAT := arm-linux-androideabi-

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

TARGET := lua

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
