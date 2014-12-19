CC=gcc
CFLAGS= -g -Wall -iquote ../ -I../include 
LIB_DEP=../libphoenix.a


all:
	$(CC) $(CFLAGS) -o test_write test_write.c matrix.c matrix.h $(LIB_DEP) 
	$(CC) $(CFLAGS) -o test_read test_read.c matrix.c matrix.h $(LIB_DEP) 


clean:
	rm -f *.o
	rm -f test_write
	rm -f test_read

restartclean:
	rm -rf /mnt/ramdisk/*
