IDIR = ~/src/include
LDIR = ~/src/lib
LIB = ${LDIR}/lib-ds.a
CXXFLAGS = -pg 
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

graph.o :     ${IDIR}/stdinc.h ${IDIR}/graph.h
wgraph.o :    ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/wgraph.h
digraph.o :   ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h
wdigraph.o :  ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h \
	      ${IDIR}/wdigraph.h
flograph.o :  ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h \
	      ${IDIR}/flograph.h
wflograph.o : ${IDIR}/stdinc.h ${IDIR}/graph.h ${IDIR}/digraph.h \
	      ${IDIR}/flograph.h ${IDIR}/wflograph.h

all:	graph.o wgraph.o digraph.o wdigraph.o flograph.o wflograph.o
	ar -ru ${LIB} graph.o wgraph.o digraph.o wdigraph.o \
		flograph.o wflograph.o
	make -C dvr IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
                    CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f *.o
	make -C dvr clean
