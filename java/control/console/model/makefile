JAVAROOT=~/bin/java
JAVAC=javac
CLASSFILES=	AdminProfile.class Comtree.class \
			Iface.class IfaceTableModel.class \
			ComtreeTableModel.class\
			Link.class LinkTableModel.class\
			Route.class RouteTableModel.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} -d ${JAVAROOT} $<

all: ${CLASSFILES}

clean: 
	rm -f *.class
