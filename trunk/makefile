CXXFLAGS = -O2 -m64

%.o: %.cpp
	${CXX} ${CXXFLAGS} -I support -c $<

HFILES = stdinc.h CommonDefs.h RouterCore.h IoProcessor.h \
	 LinkTable.h ComtreeTable.h RouteTable.h StatsModule.h \
	 QuManager.h CpAttr.h CpType.h CtlPkt.h \
	 PacketHeader.h PacketStore.h Avatar.h Monitor.h \
	 support/Misc.h support/UiList.h support/UiDlist.h support/UiListSet.h \
	 support/UiHashTbl.h support/ModHeap.h

LIBFILES = CommonDefs.o IoProcessor.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o CpAttr.o CpType.o CtlPkt.o \
	   QuManager.o PacketHeader.o PacketStore.o 

lnkTbl.o :       ${HFILES}
ComtreeTable.o : ${HFILES}
RouteTable.o :   ${HFILES}
PacketStore.o :  ${HFILES}
QuManager.o :    ${HFILES}
IoProcessor.o :  ${HFILES}
StatsModule.o :  ${HFILES}
RouterCore.o :   ${HFILES}
Host.o :         ${HFILES}
Avatar.o :       ${HFILES}
Monitor.o :      ${HFILES}
CpAttr.o :       ${HFILES}
CpType.o :       ${HFILES}
CtlPkt.o :       ${HFILES}

all : fHost fRouter fAvatar fMonitor
	cp fHost fRouter fAvatar fMonitor ${HOME}/bin

Support:
	${MAKE} -C support

RemoteDisplay:
	${MAKE} -C remoteDisplay

fRouter : RouterCore.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fHost : Host.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fAvatar : Avatar.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fMonitor : Monitor.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

lib : ${LIBFILES}
	${MAKE} -C support lib
	ar -ru lib ${LIBFILES}

clean :
	${MAKE} -C support clean
	rm -f *.o lib fHost fRouter Avatar Monitor
