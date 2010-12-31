IDIR = ~/src/include
LDIR = ~/src/lib
LIB = ${LDIR}/lib-ds.a
CXXFLAGS = -pg
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $*.cpp 

dlist.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/dlist.h
list.o :    ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/list.h
clist.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/clist.h
rlist.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/rlist.h
listset.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/listset.h
lists.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/lists.h
prtn.o :    ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/prtn.h
hashTbl.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/hashTbl.h

all:	dlist.o list.o clist.o rlist.o prtn.o lists.o listset.o hashTbl.o
	ar -ru ${LIB} dlist.o list.o rlist.o clist.o prtn.o lists.o \
	listset.o hashTbl.o
	make -C dvr IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
                    CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f *.o
	make -C dvr clean
