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
CFLAGS += -I. -I./include

objects :=
objects := parser/acceler.o parser/grammar1.o parser/listnode.o
objects += parser/node.o parser/parser.o parser/parsetok.o
objects += parser/tokenizer.o parser/bitset.o parser/metagrammar.o
objects += parser/myreadline.o

objects += objects/abstract.o objects/bufferobject.o objects/cellobject.o
objects += objects/classobject.o objects/cobject.o objects/complexobject.o
objects += objects/descrobject.o objects/fileobject.o objects/floatobject.o
objects += objects/frameobject.o objects/funcobject.o objects/intobject.o
objects += objects/iterobject.o objects/listobject.o objects/longobject.o
objects += objects/dictobject.o objects/methodobject.o objects/moduleobject.o
objects += objects/object.o objects/rangeobject.o objects/sliceobject.o
objects += objects/stringobject.o objects/structseq.o objects/tupleobject.o
objects += objects/typeobject.o objects/weakrefobject.o objects/unicodeobject.o
objects += objects/unicodectype.o

objects += python/bltinmodule.o python/exceptions.o python/ceval.o
objects += python/mysnprintf.o

# objects += parser/firstsets.o parser/grammar.o parser/pgen.o
# objects += parser/printgrammar.o

objects += python/compile.o python/codecs.o python/errors.o
objects += python/frozen.o python/future.o
#python/frozenmain.o 
objects += python/getargs.o python/getcompiler.o python/getcopyright.o
objects += python/getmtime.o python/getplatform.o python/getversion.o
objects += python/graminit.o python/import.o python/importdl.o
objects += python/marshal.o python/modsupport.o python/mystrtoul.o
objects += python/pyfpe.o python/pystate.o python/pythonrun.o
objects += python/structmember.o python/symtable.o python/sysmodule.o
objects += python/traceback.o python/getopt.o 
objects += python/dynload_win.o
#python/dynload_shlib.o
objects += python/thread.o 
 
objects += modules/config.o modules/getpath.o modules/main.o
objects += modules/gcmodule.o 
objects += modules/threadmodule.o modules/signalmodule.o
#objects += modules/posixmodule.o 
objects += modules/errnomodule.o
objects += modules/_sre.o modules/newmodule.o
objects += modules/symtablemodule.o modules/xxsubtype.o

objects += modules/getbuildinfo.o

TARGET := python222

all : $(TARGET)

$(TARGET) : libpython.a
	$(CC) $(CFLAGS) -o $@ modules/python.c libpython.a -lm
	$(STRIP) $@

libpython.a : $(objects)
	$(AR) $@ $(objects) 
	$(RANLIB) $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(objects) *.a *.exe $(TARGET)
