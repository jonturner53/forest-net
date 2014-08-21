SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = ..
AROOT = ${FROOT}/../../grafalgo/cpp
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall -Wno-c++11-extensions
CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -pthread -O2 -std=c++0x
JAVAC := javac
IDIR := ${FROOT}/include

HFILES = ${IDIR}/IfaceTable.h ${IDIR}/LinkTable.h \
	${IDIR}/ComtreeTable.h ${IDIR}/RouteTable.h ${IDIR}/StatsModule.h \
	${IDIR}/QuManager.h ${IDIR}/RouterInProc.h \
	${IDIR}/RouterOutProc.h ${IDIR}/RouterControl.h
OFILES = IfaceTable.o LinkTable.o ComtreeTable.o \
	RouteTable.o StatsModule.o QuManager.o \
	RouterInProc.o RouterOutProc.o RouterControl.o
XFILES = Router

${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB} ${XFILES}
	cp ${XFILES} ${BIN}

${FLIB} : ${OFILES}
	ar -ru ${FLIB} ${OFILES}

Router : Router.o ${LIBS} RouterInProc.o RouterControl.o RouterOutProc.o
	${CXX} ${CXXFLAGS} $< RouterInProc.o RouterControl.o \
	RouterOutProc.o ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
