SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = ../..
AROOT = ${FROOT}/../../grafalgo/cpp
IDIR = ${FROOT}/include
INCLUDES := -I ${IDIR} -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN := ~/bin
CXXFLAGS := -Wall ${ARCH} ${INCLUDES} -O2 -std=c++0x
JAVAC := javac

HFILES = ${IDIR}/Forest.h ${IDIR}/RateSpec.h ${IDIR}/CtlPkt.h ${IDIR}/Packet.h \
	 ${IDIR}/PacketLog.h ${IDIR}/PacketStore.h  ${IDIR}/PacketStoreTs.h \
	 ${IDIR}/Queue.h ${IDIR}/Quu.h ${IDIR}/Lfq.h ${IDIR}/Np4d.h \
	${IDIR}/Packet.h ${IDIR}/Logger.h \
	${IDIR}/PacketFilter.h ${IDIR}/Repeater.h ${IDIR}/RepeatHandler.h
OFILES = testQuu.o testLfq.o testLfq11.o
XFILES = testQuu testLfq testLfq11

${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -pthread -c $<

all : ${XFILES}

testQuu : testQuu.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

testLfq : testLfq.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

testLfq11 : testLfq11.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o
