JAVAROOT = ~/bin/java
ALGOLIBROOT = ../../algoLib
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
CXXFLAGS = -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

LIBFILES = NetInfo.o
XFILES = NetMgr CliMgr ComtCtl Console BuildRtables
CLASSFILES = NetInfo.class TestNetInfo.class ComtreeDisplay.class

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

%.class: %.java
	javac -cp ${JAVAROOT} $<
	mv *.class ${JAVAROOT}/forest/control

all : lib ${CLASSFILES} ${XFILES}

lib : ${LIBFILES}
	ar -ru ${FLIB} ${LIBFILES}

NetMgr : NetMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${HOME}/bin

Console : Console.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

CliMgr : ClientMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${HOME}/bin

ComtCtl : ComtCtl.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${HOME}/bin

BuildRtables : BuildRtables.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

clean :
	rm -f *.o *.class ${XFILES}
