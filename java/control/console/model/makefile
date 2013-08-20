JAVAROOT=~/bin/java
JAVAC=javac
CLASSFILES=	AdminProfile.class ComtreeTable.class ComtreeTableModel.class\
			IfaceTable.class IfaceTableModel.class \
			LinkTable.class LinkTableModel.class\
			RouteTable.class RouteTableModel.class

%.class: %.java
	${JAVAC} -cp ${JAVAROOT} -d ${JAVAROOT} $<

all: ${CLASSFILES}

clean: 
	rm -f *.class
