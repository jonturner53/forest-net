IDIR = ../../include
LDIR = ../../lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg 

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $*.cpp 

altPath.o :   ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h \
	      ${IDIR}/dlist.h ${IDIR}/altPath.h
faltPath.o :  ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h \
	      ${IDIR}/dlist.h ${IDIR}/faltPath.h
edmonds.o :   ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h \
	      ${IDIR}/rlist.h ${IDIR}/prtn.h ${IDIR}/edmonds.h
flowMatch.o : ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h ${IDIR}/dlist.h \
	      ${IDIR}/wflograph.h ${IDIR}/dheap.h 
match.o :     ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/dlist.h
matchRep.o :  ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/dlist.h

all: ${LIBGA} xfiles

clean:
	rm -f *.o *.exe

${LIBGA}:altPath.o faltPath.o edmonds.o flowMatch.o
	ar -ru ${LIBGA} altPath.o faltPath.o edmonds.o flowMatch.o

xfiles:	match matchRep

match: match.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

matchRep: matchRep.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
