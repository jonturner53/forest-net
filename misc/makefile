IDIR = ../include
LDIR = ../lib
LIB = ${LDIR}/lib-misc.a

CXXFLAGS = -pg
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

all : ${LIB}

misc.o : ${IDIR}/stdinc.h ${IDIR}/misc.h

${LIB} : misc.o
	ar -ru ${LIB} misc.o

clean :
	rm -f misc.o ${LIB}
