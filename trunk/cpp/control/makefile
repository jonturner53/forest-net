SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = $(shell pwd)/../
AROOT = ${FROOT}/../../algolib-jst
INCLUDES := -I ${FROOT}/include -I ${AROOT}/include
FLIB := ${FROOT}/lib/lib-forest.a
LIBS := ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN := ~/bin
WARN := -Wall -Wno-c++11-extensions

#CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -O2 -std=c++11 -stdlib=libc++
ifeq (${DB},1)
	LDFLAG = -pthread -lmysqlcppconn
	CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -O2 -std=c++0x -DDB_MODE
else
	LDFLAG = -pthread
	CXXFLAGS := ${WARN} ${ARCH} ${INCLUDES} -O2 -std=c++0x
endif
JAVAC := javac
IDIR := ${FROOT}/include


HFILES = ${IDIR}/NetInfo.h ${IDIR}/ComtInfo.h ${IDIR}/CpHandler.h
ifeq (${DB},1)
	OFILES = NetInfo.o ComtInfo.o CpHandler.o Substrate.o ClientTable.o DBConnector.o
else
	OFILES = NetInfo.o ComtInfo.o CpHandler.o Substrate.o ClientTable.o AdminTable.o
endif
XFILES = NetMgr ClientMgr ComtCtl BuildRtables AvatarServer PhotoServer #threadExample Console 

${OFILES} : ${HFILES}

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : ${FLIB} ${XFILES}
	cp ${XFILES} ${BIN}

${FLIB} : ${OFILES}
	ar -ru ${FLIB} ${OFILES}

NetMgr : NetMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} ${LDFLAG} -o $@

foo : foo.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@

threadExample : threadExample.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -v -o $@

Console : Console.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

ClientMgr : ClientMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@

ComtCtl : ComtCtl.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@

PhotoServer : PhotoServer.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@	

AvatarServer : AvatarServer.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@	
	
BuildRtables : BuildRtables.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clean :
	rm -f *.o ${XFILES}
