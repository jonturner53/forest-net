ALGOLIBROOT = ../../algolib-jst
JAVAROOT = ~/bin/java
JAVAC=/usr/java/latest/bin/javac
CLASSFILES = StdDraw.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} $<
	mv *.class ${JAVAROOT}/princeton

all : ${CLASSFILES}

clean :
	rm -f *.class
