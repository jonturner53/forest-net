ALGOLIBROOT = ../../../algolib-jst
BIN = ~/bin
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
LIBFILES = IoProcessor.o IfaceTable.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o QuManager.o
CXXFLAGS = -Wall -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include
XFILES = Router

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

all : lib ${XFILES}

Router : RouterCore.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -v -o $@
	cp $@ ${BIN}

lib : ${LIBFILES}
	ar -ru ${FLIB} ${LIBFILES}

clean :
	rm -f *.o lib Router
