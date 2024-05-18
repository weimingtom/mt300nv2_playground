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

objects := tinybasic.o

TARGET := tinybasic

all : $(TARGET)

$(TARGET) : $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) -lm

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(objects) *.a *.exe *.o $(TARGET)
