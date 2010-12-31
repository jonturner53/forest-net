IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

graph_d.o :     ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/misc.h
wgraph_d.o :    ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/wgraph.h \
		${IDIR}/misc.h
digraph_d.o :   ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h \
		${IDIR}/misc.h
wdigraph_d.o :  ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h \
		${IDIR}/wdigraph.h ${IDIR}/misc.h
flograph_d.o :  ${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/digraph.h \
		${IDIR}/graph.h ${IDIR}/misc.h
wflograph_d.o : ${IDIR}/stdinc.h ${IDIR}/wflograph.h ${IDIR}/flograph.h \
		${IDIR}/digraph.h ${IDIR}/graph.h ${IDIR}/misc.h

all:	graph_d wgraph_d digraph_d wdigraph_d flograph_d wflograph_d

clean:
	rm -f *.o *.exe

graph_d: graph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

wgraph_d: wgraph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

digraph_d: digraph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

wdigraph_d: wdigraph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

flograph_d: flograph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

wflograph_d: wflograph_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
