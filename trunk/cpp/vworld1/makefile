ALGOLIBROOT = ../../algolib-jst
JAVAC=/usr/java/latest/bin/javac
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
XFILES = Avatar Avatar-v0 Monitor genWalls cliProxy
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

all : ${XFILES}

Avatar : Avatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

Avatar-v0 : Avatar-v0.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

Monitor : Monitor.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

genWalls : genWalls.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
cliProxy : ClientProxy.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

clean :
	rm -f *.o 
