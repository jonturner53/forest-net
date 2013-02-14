JAVAROOT = ${BIN}/java
ALGOLIBROOT = ../../algolib-jst
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include -I /usr/include/mysql

LIBFILES = NetInfo.o ComtInfo.o
XFILES = NetMgr CliMgr CliMgr-v0 ComtCtl Console BuildRtables #SqlProxy

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

all : lib ${XFILES}

lib : ${LIBFILES}
	ar -ru ${FLIB} ${LIBFILES}

NetMgr : NetMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${BIN}

Console : Console.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${BIN}

CliMgr : ClientMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${BIN}

CliMgr-v0 : ClientMgr-v0.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${BIN}

ComtCtl : ComtCtl.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${BIN}
SqlProxy : SqlProxy.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} libmysqlpp.a libmysqlclient.a -lssl -o $@
	cp $@ ${BIN}
BuildRtables : BuildRtables.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${BIN}

clean :
	rm -f *.o ${XFILES}
