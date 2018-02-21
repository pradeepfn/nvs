FC=mpif90
CC=g++
MPICPLUS=mpic++
CFLAGS=-g  -std=c++11 -I../../nvstream/include
CPPFLAGS = -DNV_STREAM -DMPI_ENABLED
#CPPFLAGS = -DNV_STREAM
FFLAGS=-g
LDFLAGS=-g ../../nvstream/build/src/libyuma.a ../../nvstream/third-party/nvml/src/debug/libpmemlog.a ../../nvstream/third-party/nvml/src/debug/libpmemobj.a ../../nvstream/third-party/nvml/src/debug/libpmem.a -lpthread -lboost_thread -lboost_log -lboost_filesystem -lboost_system -lrt -lstdc++ -ldl


LDCPLUSFLAGS=-g ../../nvstream/build/src/libyuma.a ../../nvstream/third-party/nvml/src/debug/libpmemlog.a ../../nvstream/third-party/nvml/src/debug/libpmemobj.a ../../nvstream/third-party/nvml/src/debug/libpmem.a -lpthread -lboost_thread -lboost_log -lboost_filesystem -lboost_system -lrt -ldl

#LDFLAGS=-g  -lpthread -lssl -lcrypto

all: p_writer p_reader micro_writer

p_writer: parallel_writer.o allocate.o wrapper.o
	$(FC) -o $@ $^ $(LDFLAGS)

p_reader: parallel_reader.o
	$(MPICPLUS) -o $@ $^ $(LDCPLUSFLAGS)

micro_writer: micro_writer.o
	$(MPICPLUS) -o $@ $^ $(LDCPLUSFLAGS)

parallel_writer.o: parallel_writer.f90 allocate.o
	$(FC) $(FFLAGS) -c $<


parallel_reader.o: parallel_reader.cc
	$(MPICPLUS) $(CFLAGS) -c $<


micro_writer.o: micro_writer.cc
	$(MPICPLUS) $(CFLAGS) $(CPPFLAGS) -c $<

allocate.o: allocate.f90
	$(FC) $(FFLAGS) -c $<


wrapper.o : wrapper.cc
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf p_reader p_writer micro_writer micro_writer.o parallel_reader.o parallel_writer.o wrapper.o allocate.o allocator.mod global.mod

rclean:
