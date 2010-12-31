IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

cycRed.o :   ${IDIR}/stdinc.h ${IDIR}/list.h ${IDIR}/flograph.h \
	     ${IDIR}/cycRed.h
lcap.o :     ${IDIR}/stdinc.h ${IDIR}/list.h ${IDIR}/dheap.h \
             ${IDIR}/flograph.h ${IDIR}/lcap.h
mcFlo.o :    ${IDIR}/stdinc.h ${IDIR}/flograph.h
mcFloRep.o : ${IDIR}/stdinc.h ${IDIR}/flograph.h
check.o :    ${IDIR}/stdinc.h ${IDIR}/list.h ${IDIR}/flograph.h

all: ${LIBGA} xfiles

clean:
	rm -f *.o *.exe

${LIBGA}:cycRed.o lcap.o 
	ar -ru ${LIBGA} cycRed.o lcap.o 

xfiles:	mcFlo mcFloRep check

mcFlo: mcFlo.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

mcFloRep: mcFloRep.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

check: check.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

