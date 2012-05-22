ALGOLIBROOT = ../../algolib-jst
JAVAROOT = ~/bin/java
FLIB = ../lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-util.a ${ALGOLIBROOT}/lib/lib-ds.a
LIBFILES = CommonDefs.o CpAttr.o CpType.o CtlPkt.o PacketHeader.o \
	   PacketLog.o PacketStore.o  PacketStoreTs.o \
	   Queue.o Np4d.o Heap.o HeapSet.o Misc.o
CLASSFILES = Forest.class
CXXFLAGS = -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

%.class: %.java
	javac -cp ${JAVAROOT} $<
	mv *.class ${JAVAROOT}/forest/common

all : ${LIBFILES} ${CLASSFILES}
	ar -ru ${FLIB} ${LIBFILES}

clean :
	rm -f *.o *.class
