JAVAROOT = ~/bin/java
JAVAC=javac
CLASSFILES = Forest.class CpAttr.class CpTyp.class CtlPkt.class \
	     PktHeader.class Heap.class RateSpec.class Pair.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} $<
	cp *.class ${JAVAROOT}/forest/common

all : ${CLASSFILES}

clean :
	rm -f *.class
