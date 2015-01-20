CFLAGS = -g -I/home/pradeep/checkout/phoenix/include 
LDFLAGS = -L/home/pradeep/checkout/phoenix/lib -lphoenix 

CC = mpif90

all:
	$(CC) -c allocate.f90 $(CFLAGS)
	$(CC) -c write.f90 $(CFLAGS)
	$(CC) -c read.f90 $(CFLAGS)
	$(CC) write.o allocate.o $(LDFLAGS) -o write	
	$(CC) read.o allocate.o $(LDFLAGS) -o read

clean:
	rm -rf write
	rm -rf *.o
	rm -rf *.mod

restartclean:
	rm -rf /mnt/ramdisk/*
