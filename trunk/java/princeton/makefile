JAVAROOT = ~/bin/java
JAVAC=javac
CLASSFILES = StdDraw.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} $<
	mv *.class ${JAVAROOT}/princeton

all : ${CLASSFILES}

clean :
	rm -f *.class
