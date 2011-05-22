CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

HFILES = stdinc.h Misc.h Np4d.h UiList.h UiDlist.h listset.h hashTbl.h mheap.h
OFILES = Misc.o Np4d.o UiList.o UiDlist.o listset.o hashTbl.o mheap.o

UiList.o :    ${HFILES}
UiDlist.o :   ${HFILES}
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
