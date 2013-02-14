ALGOLIBROOT = ../../algolib-jst
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

all : Host

Host : Host.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${BIN}

clean :
	rm -f *.o Host
