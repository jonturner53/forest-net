CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

HFILES = stdinc.h Misc.h Np4d.h list.h dlist.h listset.h hashTbl.h mheap.h
OFILES = Misc.o Np4d.o list.o dlist.o listset.o hashTbl.o mheap.o

list.o :    ${HFILES}
dlist.o :   ${HFILES}
listset.o : ${HFILES}
hashTbl.o : ${HFILES}
mheap.o :   ${HFILES}
Misc.o :    ${HFILES}
Np4d.o :    ${HFILES}

lib : ${OFILES}
	ar -ru ../lib ${OFILES}
	touch timestamp

clean :
	rm -f *.o
