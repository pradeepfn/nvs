CFLAGS = -g -fbacktrace -I/home/pradeep/checkout/phoenix/include -std=legacy 
LDFLAGS = -g -L/home/pradeep/checkout/phoenix/lib -lphoenix 

CC = mpif90

all:
	$(CC) -c allocate.f90 $(CFLAGS)
	$(CC) -c write.f90 $(CFLAGS)
	$(CC) -c read.f90 $(CFLAGS)
	$(CC) -c addition.f90 $(CFLAGS)
	$(CC) write.o allocate.o $(LDFLAGS) -o write	
	$(CC) read.o allocate.o $(LDFLAGS) -o read
	$(CC) addition.o allocate.o $(LDFLAGS) -o addition

mpi:
	

clean:
	rm -rf write
	rm -rf *.o
	rm -rf *.mod

restartclean:
	rm -rf /mnt/ramdisk/addition/mmap*
