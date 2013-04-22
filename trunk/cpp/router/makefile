SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = $(shell pwd)/../
AROOT = ${FROOT}/../../algolib-jst
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall -Wno-c++11-extensions
CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -O2
JAVAC := javac
IDIR := ${FROOT}/include

HFILES = ${IDIR}/IoProcessor.h ${IDIR}/IfaceTable.h ${IDIR}/LinkTable.h \
	${IDIR}/ComtreeTable.h ${IDIR}/RouteTable.h ${IDIR}/StatsModule.h \
	${IDIR}/QuManager.h
OFILES = IoProcessor.o IfaceTable.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o QuManager.o
XFILES = Router

${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB} ${XFILES}
	cp ${XFILES} ${BIN}

${FLIB} : ${OFILES}
	ar -ru ${FLIB} ${OFILES}

Router : RouterCore.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
