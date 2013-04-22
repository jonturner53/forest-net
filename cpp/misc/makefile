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

XFILES = Host

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${XFILES}
	cp ${XFILES} ${BIN}

Host : Host.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
