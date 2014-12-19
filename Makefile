IDIR =include
CC=gcc
CFLAGS= -g -Wall -I$(IDIR)
LDFLAGS=

_DEPS = phoenix.h
_LIB_OBJ = px_checkpoint.o px_log.o px_util.o px_read.o px_debug.o
_LIB_HEADER = px_checkpoint.h px_log.h px_util.h px_read.h px_debug.h

SRC=src
ODIR=obj
LDIR =lib

LIBS=-lm
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
LIB_OBJ = $(patsubst %,$(ODIR)/%,$(_LIB_OBJ))
LIB_HEADER = $(patsubst %,$(SRC)/%,$(_LIB_HEADER))

all: lib

lib: ${LIB_OBJ} ${LIB_HEADER}
	ar -cvq libphoenix.a $^

$(ODIR)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: dirs clean

clean: 
	rm -f $(ODIR)/*.o librvm.a $(TEST_OBJ) 
	rm -f libphoenix.a
