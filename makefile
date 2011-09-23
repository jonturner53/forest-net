CXXFLAGS = -O2 -m64 -I . -I support

LIBS = lib support/lib

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

LIBFILES = CommonDefs.o IoProcessor.o IfaceTable.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o CpAttr.o CpType.o CtlPkt.o \
	   QuManager.o PacketHeader.o PacketStore.o  PacketStoreTs.o \
	   NetInfo.o ComtreeController_NetInfo.o

all : supportLib fHost fRouter fAvatar fMazeAvatar fClientAvatar fMonitor \
	fCliMgr fNetMgr fComtreeController fComtreeController_NetInfo \
	fConsole BuildRtables

supportLib:
	${MAKE} -C support

fRouter : RouterCore.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

fHost : Host.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

fAvatar : Avatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fMazeAvatar : MazeAvatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fClientAvatar : ClientAvatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fMonitor : Monitor.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fNetMgr : NetMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -lpthread -o $@
	cp $@ ${HOME}/bin
fConsole : Console.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fCliMgr : ClientMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin
fComtreeController : ComtreeController.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

fComtreeController_NetInfo : ComtreeController_NetInfo.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

BuildRtables : BuildRtables.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

lib : ${LIBFILES}
	ar -ru lib ${LIBFILES}

clean :
	${MAKE} -C support clean
	rm -f *.o lib fHost fRouter fAvatar fMonitor fNetMgr fCli fCliMgr fComtreeController fComtreeController_NetInfo
