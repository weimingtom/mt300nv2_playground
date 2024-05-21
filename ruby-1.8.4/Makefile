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

OBJECTS :=
OBJECTS += array.o bignum.o class.o compar.o dir.o 
OBJECTS += dln.o enum.o error.o eval.o file.o gc.o 
OBJECTS += hash.o inits.o io.o marshal.o math.o 
OBJECTS += numeric.o object.o pack.o parse.o 
OBJECTS += process.o prec.o random.o range.o 
OBJECTS += re.o regex.o ruby.o signal.o sprintf.o 
OBJECTS += st.o string.o struct.o time.o util.o 
OBJECTS += variable.o version.o  dmyext.o

OBJECTS += win32/win32.o

TARGET := ruby184

all : $(TARGET)

$(TARGET) : libruby.a
	$(CC) $(CFLAGS) -o $@ main.c libruby.a -lws2_32 -lm
	
#$(STRIP) $@

# -lmsimg32 for AlphaBlend@44
# -lc

libruby.a : $(OBJECTS)
	$(AR) $@ $(OBJECTS) 
	$(RANLIB) $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(OBJECTS) *.a *.exe $(TARGET)
