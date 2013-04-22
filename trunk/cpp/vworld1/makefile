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

XFILES = Avatar-v0 Monitor genWalls # Avatar cliProxy

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB} ${XFILES}
	cp ${XFILES} ${BIN}

Avatar : Avatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

Avatar-v0 : Avatar-v0.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

Monitor : Monitor.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

genWalls : genWalls.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

cliProxy : ClientProxy.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
