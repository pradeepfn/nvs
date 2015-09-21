CC=mpicc
CFLAGS= -g -Wall -pthread -iquote ../ -I../include  
LDFLAGS= -L../lib -lphoenix -larmci


all:
	$(CC) $(CFLAGS) -o test_write test_write.c matrix.c matrix.h $(LDFLAGS) 
	$(CC) $(CFLAGS) -o test_read test_read.c matrix.c matrix.h $(LDFLAGS) 


clean:
	rm -f *.o
	rm -f test_write
	rm -f test_read

restartclean:
	rm -rf /mnt/ramdisk/mmap.file.*
