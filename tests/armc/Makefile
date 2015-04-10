CFLAGS = -g -I../../include 
LDFLAGS = -L../../lib -lphoenix -larmci 

CC = mpicc

all:
	$(CC) -o remotecopy main.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf remotecopy

restartclean:
