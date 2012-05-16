ALGOLIBROOT = ../../algoLib
JAVAROOT = ~/bin/java
CLASSFILES = StdDraw.class

%.class: %.java
	javac -cp ${JAVAROOT} $<
	mv *.class ${JAVAROOT}/princeton

all : ${CLASSFILES}

clean :
	rm -f *.class
