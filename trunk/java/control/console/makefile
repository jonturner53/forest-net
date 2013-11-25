JAVAROOT=~/bin/java
JAVAC=javac
CLASSFILES=	model/AdminProfile.class model/Comtree.class\
			model/ComtreeTableModel.class\
			model/Iface.class model/IfaceTableModel.class \
			model/Link.class model/LinkTableModel.class\
			model/Route.class model/RouteTableModel.class\
			model/LogFilter.class model/LogFilterTableModel.class\
			model/Circle.class model/Line.class\
			model/Rect.class\
			ConnectionNetMgr.class ConnectionComtCtl.class\
			ComtreeDisplay.class\
			ChangePwdDialog.class ConnectDialog.class\
			LoginDialog.class UpdateProfileDialog.class\
			LogFrame.class\
			NetMgrConsole.class
                        
%.class: %.java
	${JAVAC} -cp ${JAVAROOT} -d ${JAVAROOT} $<

all: ${CLASSFILES}

clean: 
	rm -f *.class
