CLASSFILES = Common.class MenuItem.class TopoComponent.class TopoLink.class Ports.class Panel.class TopologyGen.class

%.class: %.java
	echo $$CLASSPATH $$HOME
	javac -cp ../.. $<
	cp $@ ~/bin/java/TopologyGen

all : ${CLASSFILES}

clean :
	rm -f *.class
