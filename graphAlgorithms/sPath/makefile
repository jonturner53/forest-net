IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

dijkstra.o:    ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/dheap.h 
badCase.o:     ${IDIR}/stdinc.h ${IDIR}/wdigraph.h
bfScan.o:      ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h ${IDIR}/dheap.h
dijkstraAll.o: ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/dheap.h 
floyd.o:       ${IDIR}/stdinc.h ${IDIR}/wdigraph.h
spt.o:         ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
sptRep.o:      ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
allPairs.o:    ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
allPairsRep.o: ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
sptUpdate.o:   ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
sptUpdate2.o:  ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
check.o:       ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h

all : ${LIBGA} xfiles

clean:
	rm -f *.o *.exe

${LIBGA}: dijkstra.o bfScan.o dijkstraAll.o floyd.o
	ar -ru ${LIBGA} dijkstra.o bfScan.o dijkstraAll.o floyd.o

xfiles:	badCase spt sptRep allPairs allPairsRep check sptUpdate

spt: spt.o ${LIBS} 
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

sptRep: sptRep.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

badCase: badCase.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

allPairs: allPairs.o ${LIBS} 
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

allPairsRep: allPairsRep.o ${LIBS} 
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

check: check.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

sptUpdate: sptUpdate.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

