
CLASSFILES = topoComponent.class Panel.class topologyGen.class

%.class: %.java
	echo $$CLASSPATH $$HOME
	javac $<
	cp $@ ~/bin/java/topologyGen

all : ${CLASSFILES}

clean :
	rm -f *.class
