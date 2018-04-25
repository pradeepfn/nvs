CC   = mpicc
LD   = mpicc
CPPFLAGS = -I.
YFLAGS = -D_YUMA 
CFLAGS = $(YFLAGS) -O3 -I../nvstream/include
LDFLAGS =
LDLIBS = ../nvstream/build/src/libyuma.a ../nvstream/third-party/nvml/src/debug/libpmemlog.a ../nvstream/third-party/nvml/src/debug/libpmemobj.a ../nvstream/third-party/nvml/src/debug/libpmem.a -lpthread -lboost_thread -lboost_log -lboost_filesystem -lboost_system -lrt -lstdc++ -ldl -lm

EXEC = miniAMR.x

OBJS = block.o check_sum.o comm_block.o comm.o comm_parent.o comm_refine.o \
       comm_util.o driver.o init.o main.o move.o pack.o plot.o profile.o \
       rcb.o refine.o stencil.o target.o util.o wrapper.o

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

%.o:    %.cc
	    g++ -std=c++11 -I../nvstream/include -c $<

clean:
	rm *.o miniAMR.x

# need dependencies

block.o: block.h proto.h comm.h timer.h

check_sum.o: block.h comm.h proto.h

comm_block.o: block.h comm.h proto.h

comm.o: block.h comm.h proto.h timer.h

comm_parent.o: block.h comm.h proto.h

comm_refine.o: block.h comm.h proto.h

comm_util.o: block.h comm.h proto.h

driver.o: block.h comm.h proto.h timer.h

init.o: block.h proto.h comm.h

main.o: block.h comm.h proto.h param.h

move.o: block.h comm.h proto.h

pack.o: block.h comm.h proto.h

plot.o: block.h comm.h proto.h

profile.o: block.h comm.h proto.h timer.h

rcb.o: block.h comm.h proto.h timer.h

refine.o: block.h comm.h proto.h timer.h

stencil.o: block.h comm.h proto.h

target.o: block.h comm.h proto.h timer.h

util.o: block.h comm.h proto.h
