SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = ..
AROOT = ${FROOT}/../../grafalgo/cpp
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall
CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -pthread -O2 -std=c++0x
JAVAC := javac
IDIR := ${FROOT}/include

HFILES = ${IDIR}/Forest.h ${IDIR}/RateSpec.h ${IDIR}/CtlPkt.h ${IDIR}/Packet.h \
	 ${IDIR}/PacketLog.h ${IDIR}/PacketStore.h  ${IDIR}/PacketStoreTs.h \
	 ${IDIR}/Queue.h ${IDIR}/Lfq.h ${IDIR}/Np4d.h \
	${IDIR}/Packet.h ${IDIR}/Logger.h \
	${IDIR}/PacketFilter.h ${IDIR}/Repeater.h ${IDIR}/RepeatHandler.h
OFILES = Forest.o RateSpec.o CtlPkt.o Packet.o PacketLog.o PacketStore.o \
	 PacketStoreTs.o Queue.o Lfq.o Np4d.o Packet.o \
	 Logger.o NetBuffer.o PacketFilter.o Repeater.o RepeatHandler.o
${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB}

${FLIB} : ${OFILES}
	ar -ru ${FLIB} ${OFILES}

clean :
	rm -f *.o
