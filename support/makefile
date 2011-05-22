CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

HFILES = stdinc.h Misc.h Np4d.h UiList.h UiDlist.h UiListSet.h UiHashTbl.h ModHeap.h
OFILES = Misc.o Np4d.o UiList.o UiDlist.o UiListSet.o UiHashTbl.o ModHeap.o

UiList.o :    ${HFILES}
UiDlist.o :   ${HFILES}
UiListSet.o : ${HFILES}
UiHashTbl.o : ${HFILES}
ModHeap.o :   ${HFILES}
Misc.o :    ${HFILES}
Np4d.o :    ${HFILES}

lib : ${OFILES}
	ar -ru ../lib ${OFILES}
	touch timestamp

clean :
	rm -f *.o
