ALGOLIBROOT = ../../algoLib
FLIB = ../lib/lib-forest
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util ${ALGOLIBROOT}/lib/lib-ds
LIBFILES = CommonDefs.o CpAttr.o CpType.o CtlPkt.o PacketHeader.o \
	   PacketLog.o PacketStore.o  PacketStoreTs.o
CXXFLAGS = -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

${FLIB} : ${LIBFILES}
	ar -ru lib ${LIBFILES}

clean :
	rm -f *.o
