IDIR = ../include
LDIR = ../lib
LIB = ${LDIR}/lib-misc.a

CXXFLAGS = -O2
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

all : ${LIB} trimit

misc.o : ${IDIR}/stdinc.h ${IDIR}/misc.h
trimit.o : ${IDIR}/stdinc.h

${LIB} : misc.o
	ar -ru ${LIB} misc.o
	ranlib ${LIB}

trimit : trimit.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIB} -o $@
	cp trimit ${HOME}/bin

clean :
	rm -f misc.o ${LIB}
