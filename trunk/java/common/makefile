JAVAROOT = ~/bin/java
JAVAC=javac
CLASSFILES = Forest.class Heap.class RateSpec.class Pair.class \
	     NetBuffer.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} $<
	cp *.class ${JAVAROOT}/forest/common

all : ${CLASSFILES}

clean :
	rm -f *.class
