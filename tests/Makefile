CC=mpicc
CFLAGS= -g -Wall -pthread -iquote ../ -I../include -I/home1/03528/pradeepf/armci_install/include 
LDFLAGS= -L../lib -L/home1/03528/pradeepf/armci_install/lib -lphoenix -larmci


all:
	$(CC) $(CFLAGS) -o test_write test_write.c matrix.c matrix.h $(LDFLAGS) 
	$(CC) $(CFLAGS) -o test_read test_read.c matrix.c matrix.h $(LDFLAGS) 


clean:
	rm -f *.o
	rm -f test_write
	rm -f test_read

restartclean:
	rm -rf /home1/03528/pradeepf/mmaptest
