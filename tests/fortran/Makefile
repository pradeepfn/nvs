CFLAGS = -g -I../../include -I/home1/03528/pradeepf/armci_install/include 
LDFLAGS = -g -L../../lib -L/home1/03528/pradeepf/armci_install/lib -lphoenix  -larmci

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
	rm -rf addition

restartclean:
	rm -rf /home1/03528/pradeepf/mmaptest/mmap*
