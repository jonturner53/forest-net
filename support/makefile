CXXFLAGS = -O2 -m64 -I ..

LIBFILES = Misc.o Np4d.o UiList.o UiDlist.o UiListSet.o UiHashTbl.o \
	IdMap.o Heap.o Clist.o Graph.o UiSetPair.o HeapSet.o Queue.o

%.o : %.cpp %.h ../stdinc.h
	${CXX} ${CXXFLAGS} -c $<

lib : ${LIBFILES}
	ar -ru lib ${LIBFILES}

clean :
	rm -f *.o
