SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = ..
AROOT = ${FROOT}/../../grafalgo/cpp
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall 
CXXFLAGS := ${WARN} ${ARCH} -O2 -std=c++11
JAVAC := javac

XFILES = Host

.cpp.o:
	${CXX} ${CXXFLAGS} ${INCLUDES} -c $<

all : ${XFILES}
	cp ${XFILES} ${BIN}

Host : Host.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
