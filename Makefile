IDIR =include
CC=gcc
CFLAGS= -g -Wall -I$(IDIR)
LDFLAGS=

_DEPS = phoenix.h
_LIB_OBJ = px_checkpoint.o px_log.o

SRC=src
ODIR=obj
LDIR =lib

LIBS=-lm
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
LIB_OBJ = $(patsubst %,$(ODIR)/%,$(_LIB_OBJ))

all: lib

lib: ${LIB_OBJ}
	ar -cvq libphoenix.a $^

$(ODIR)/%.o: $(SRC)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: dirs clean

clean: 
	rm -f $(ODIR)/*.o librvm.a $(TEST_OBJ) 
	rm -rf $(ODIR)
