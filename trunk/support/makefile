CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

HFILES = stdinc.h misc.h list.h dlist.h listset.h hashTbl.h mheap.h
OFILES = misc.o list.o dlist.o listset.o hashTbl.o mheap.o

list.o :    ${HFILES}
dlist.o :   ${HFILES}
listset.o : ${HFILES}
hashTbl.o : ${HFILES}
mheap.o :   ${HFILES}
misc.o :    ${HFILES}

lib : ${OFILES}
	ar -ru ../lib ${OFILES}
	touch timestamp

clean :
	rm -f *.o
