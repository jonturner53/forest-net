JAVAROOT=~/bin/java
JAVAC=javac
CLASSFILES=	model/AdminProfile.class model/ComtreeTable.class\
			model/ComtreeTableModel.class\
            model/IfaceTable.class model/IfaceTableModel.class \
            model/LinkTable.class model/LinkTableModel.class\
            model/RouteTable.class model/RouteTableModel.class\
			ChangePwdDialog.class ConnectDialog.class\
			LoginDialog.class UpdateProfileDialog.class\
			NetMgrConsole.class\
			
%.class: %.java
	${JAVAC} -cp ${JAVAROOT} -d ${JAVAROOT} $<

all: ${CLASSFILES}

clean: 
	rm -f *.class
