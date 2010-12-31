IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

kruskal.o:    ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h ${IDIR}/prtn.h
prim.o:       ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h ${IDIR}/dheap.h
primF.o:      ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h ${IDIR}/fheaps.h
rrobin.o:     ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h \
	      ${IDIR}/llheaps.h ${IDIR}/prtn.h

mst.o:        ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h
mstRep.o:     ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h
mstUpdate.o:  ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h
mstUpdate2.o: ${IDIR}/stdinc.h ${IDIR}/wgraph.h ${IDIR}/list.h

all:	${LIBGA} xfiles

clean:
	rm -f *.o *.exe

${LIBGA}:kruskal.o prim.o primF.o rrobin.o
	ar -ru ${LIBGA} kruskal.o prim.o primF.o rrobin.o

xfiles:	mst mstRep check mstUpdate mstUpdate2

mst: mst.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

mstRep: mstRep.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

check: check.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

mstUpdate: mstUpdate.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

mstUpdate2: mstUpdate2.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
