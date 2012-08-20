ALGOLIBROOT = ../../algolib-jst
JAVAC=/usr/java/latest/bin/javac
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
XFILES = Avatar Monitor genWalls cliProxy
CLASSFILES = AvatarStatus.class AvatarGraphic.class ShowWorld.class \
	DriveAvatar.class ShowWalls.class 
CXXFLAGS = -Wall -O2 -m32 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

%.class: %.java
	${JAVAC} -nowarn -cp ~/bin/java $<
	cp *.class ~/bin/java/forest/vworld1

all : ${XFILES} ${CLASSFILES}

Avatar : Avatar.o ${LIBS}
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
