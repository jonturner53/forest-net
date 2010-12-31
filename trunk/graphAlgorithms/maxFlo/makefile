IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ga.a ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
LIBGA = ${LDIR}/lib-ga.a
CXXFLAGS = -pg 

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

augPath.o :   	${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/dheap.h \
	      	${IDIR}/list.h ${IDIR}/augPath.h
shortPath.o : 	${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/dheap.h \
	      	${IDIR}/list.h ${IDIR}/augPath.h ${IDIR}/shortPath.h
maxCap.o :    	${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/dheap.h \
	      	${IDIR}/list.h ${IDIR}/augPath.h ${IDIR}/maxCap.h
capScale.o :  	${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/dheap.h \
	      	${IDIR}/list.h ${IDIR}/augPath.h ${IDIR}/capScale.h
dinic.o :    	${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/list.h \
		${IDIR}/dinic.h
dinicDtrees.o : ${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/pathset.h \
	        ${IDIR}/dtrees.h ${IDIR}/list.h ${IDIR}/dinicDtrees.h
prePush.o :     ${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/list.h \
		${IDIR}/prePush.h
ppFifo.o :      ${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/list.h \
		${IDIR}/prePush.h ${IDIR}/ppFifo.h
maxFlo.o :      ${IDIR}/stdinc.h ${IDIR}/flograph.h
maxFloRep.o :   ${IDIR}/stdinc.h ${IDIR}/flograph.h
check.o :       ${IDIR}/stdinc.h ${IDIR}/flograph.h ${IDIR}/list.h
badCase.o :     ${IDIR}/stdinc.h ${IDIR}/flograph.h 

all:	${LIBGA} xfiles

clean:
	rm -f *.o *.exe

${LIBGA}:augPath.o shortPath.o maxCap.o capScale.o \
	dinic.o dinicDtrees.o prePush.o ppFifo.o 
	ar -ru ${LIBGA} augPath.o shortPath.o maxCap.o \
	capScale.o dinic.o dinicDtrees.o prePush.o \
	ppFifo.o 

xfiles:	maxFlo maxFloRep check badCase 

maxFlo: maxFlo.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

maxFloRep: maxFloRep.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

check: check.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

badCase: badCase.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
