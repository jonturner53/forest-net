ALGOLIBROOT = ../../algolib-jst
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
LIBFILES = IoProcessor.o IfaceTable.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o QuManager.o
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

all : lib Router

Router : RouterCore.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

lib : ${LIBFILES}
	ar -ru ${FLIB} ${LIBFILES}

clean :
	rm -f *.o lib Router
