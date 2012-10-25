ALGOLIBROOT = ../../algolib-jst
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
LIBFILES = CommonDefs.o CpAttr.o CpType.o CtlPkt.o PacketHeader.o \
	   PacketLog.o PacketStore.o  PacketStoreTs.o \
	   Queue.o Np4d.o Heap.o HeapSet.o Misc.o
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

all : ${LIBFILES}
	ar -ru ${FLIB} ${LIBFILES}

clean :
	rm -f *.o
