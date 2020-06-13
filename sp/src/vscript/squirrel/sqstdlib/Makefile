SQUIRREL= ..


CC?= gcc
OUT?= $(SQUIRREL)/lib/libsqstdlib.a
INCZ?= -I$(SQUIRREL)/include -I. -Iinclude
DEFS= $(CC_EXTRA_FLAGS)
LIB=

OBJS= \
	sqstdblob.o \
	sqstdio.o \
	sqstdstream.o \
	sqstdmath.o \
	sqstdsystem.o \
	sqstdstring.o \
	sqstdaux.o \
	sqstdrex.o

SRCS= \
	sqstdblob.cpp \
	sqstdio.cpp \
	sqstdstream.cpp \
	sqstdmath.cpp \
	sqstdsystem.cpp \
	sqstdstring.cpp \
	sqstdaux.cpp \
	sqstdrex.cpp


sq32:
	$(CC) -O2 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
	ar rc $(OUT) *.o
	rm *.o

sqprof:
	$(CC) -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
	ar rc $(OUT) *.o
	rm *.o

sq64:
	$(CC) -O2 -m64 -fno-exceptions -D_SQ64 -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
	ar rc $(OUT) *.o
	rm *.o
