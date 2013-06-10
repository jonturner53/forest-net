JAVAROOT = ~/bin/java
JAVAC=javac

CLASSFILES = NetInfo.class ComtInfo.class ComtreeDisplay.class AcctMgr.class

%.class: %.java
	javac -cp ${JAVAROOT} $<
	cp *.class ${JAVAROOT}/forest/control

all : ${CLASSFILES}

clean :
	rm -f *.class
