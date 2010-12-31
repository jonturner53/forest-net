IDIR = ~/src/include
LDIR = ~/src/lib
LIB = ${LDIR}/lib-ds.a
CC = g++
CXXFLAGS = -pg 
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

dheap.o :   ${IDIR}/stdinc.h ${IDIR}/dheap.h
mheap.o :   ${IDIR}/stdinc.h ${IDIR}/mheap.h
lheaps.o :  ${IDIR}/stdinc.h ${IDIR}/lheaps.h
llheaps.o : ${IDIR}/stdinc.h ${IDIR}/llheaps.h
fheaps.o :  ${IDIR}/stdinc.h ${IDIR}/fheaps.h

all :	dheap.o mheap.o lheaps.o llheaps.o fheaps.o 
	ar -ru ${LIB} dheap.o mheap.o lheaps.o llheaps.o fheaps.o 
	make -C dvr IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
                    CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f *.o
	make -C dvr clean
