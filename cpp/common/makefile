SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = $(shell pwd)/../
AROOT = ${FROOT}/../../algolib-jst
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall -Wno-c++11-extensions
CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -O2
JAVAC := javac
IDIR := ${FROOT}/include

HFILES = ${IDIR}/Forest.h ${IDIR}/RateSpec.h ${IDIR}/CtlPkt.h ${IDIR}/Packet.h \
	 ${IDIR}/PacketLog.h ${IDIR}/PacketStore.h  ${IDIR}/PacketStoreTs.h \
	 ${IDIR}/Queue.h ${IDIR}/Np4d.h ${IDIR}/Heap.h ${IDIR}/HeapSet.h \
	${IDIR}/Misc.h ${IDIR}/Packet.h ${IDIR}/Logger.h
OFILES = Forest.o RateSpec.o CtlPkt.o Packet.o PacketLog.o PacketStore.o \
	 PacketStoreTs.o Queue.o Np4d.o Heap.o HeapSet.o Misc.o Packet.o \
	 Logger.o NetBuffer.o
${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB}

${FLIB} : ${OFILES}
	ar -ru ${FLIB} ${OFILES}

clean :
	rm -f *.o
