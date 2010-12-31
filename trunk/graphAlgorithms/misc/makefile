BIN = ~/bin
IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg 

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

bfs.o :        ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h
toposort.o :   ${IDIR}/stdinc.h ${IDIR}/wdigraph.h ${IDIR}/list.h
cgraph.o :     ${IDIR}/stdinc.h ${IDIR}/graph.h
rgraph.o :     ${IDIR}/stdinc.h ${IDIR}/graph.h
nca.o :        ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/prtn.h ${IDIR}/nca.h

all:	${LIBGA} cgraph bfs toposort rgraph test_nca
	cp bfs toposort rgraph ${BIN}

clean:
	rm -f *.o *.exe
	rm -f ${BIN}/bfs.exe ${BIN}/toposort.exe ${BIN}/rgraph.exe \
	test_nca.exe

${LIBGA}:nca.o
	ar -ru ${LIBGA} nca.o

bfs: bfs.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

toposort: toposort.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

test_nca: test_nca.o ${LIBS} 
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

cgraph: cgraph.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

rgraph: rgraph.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
