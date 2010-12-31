IDIR = ~/src/include
LDIR = ~/src/lib
LIBS =  ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

dheap_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/dheap.h
mheap_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/mheap.h
llheaps_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/llheaps.h
fheaps_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/fheaps.h \
	${IDIR}/clist.h ${IDIR}/list.h
lheaps_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/lheaps.h

all:	dheap_d mheap_d llheaps_d lheaps_d fheaps_d

clean:
	rm -f *.o *.exe

dheap_d: dheap_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

mheap_d: mheap_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

llheaps_d: llheaps_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

fheaps_d: fheaps_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

lheaps_d: lheaps_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

