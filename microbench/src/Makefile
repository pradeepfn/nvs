FC=mpif90
CC=g++
CFLAGS=-g  -std=c++11 -I../../nvstream/include
FFLAGS=-g
LDFLAGS=-g ../../nvstream/build/src/libyuma.a ../../nvstream/third-party/nvml/src/debug/libpmemlog.a ../../nvstream/third-party/nvml/src/debug/libpmemobj.a ../../nvstream/third-party/nvml/src/debug/libpmem.a -lpthread -lboost_thread -lboost_log -lboost_filesystem -lboost_system -lrt -lstdc++ -ldl
#LDFLAGS=-g  -lpthread -lssl -lcrypto
main: parallel_writer.o allocate.o wrapper.o
	$(FC) -o $@ $^ $(LDFLAGS)

parallel_writer.o: parallel_writer.f90 allocate.o
	$(FC) $(FFLAGS) -c $<


allocate.o: allocate.f90
	$(FC) $(FFLAGS) -c $<


wrapper.o : wrapper.cc
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf parallel_writer.o wrapper.o allocate.o allocator.mod global.mod

rclean:
