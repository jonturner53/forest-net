
CLASSFILES = MenuItem.class NetInfo.class TopoLink.class TopoComponent.class Ports.class Panel.class TopologyGen.class

%.class: %.java
	echo $$CLASSPATH $$HOME
	javac $<
	cp $@ ~/bin/java/TopologyGen

all : ${CLASSFILES}

clean :
	rm -f *.class
