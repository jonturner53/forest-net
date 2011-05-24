CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -I support -c $<

HFILES = stdinc.h CommonDefs.h RouterCore.h IoProcessor.h \
	 LinkTable.h ComtreeTable.h RouteTable.h statsModule.h \
	 QuManager.h CpAttr.h CpType.h CtlPkt.h \
	 PacketHeader.h PacketStore.h Avatar.h Monitor.h \
	 support/Misc.h support/UiList.h support/UiDlist.h support/UiListSet.h \
	 support/UiHashTbl.h support/ModHeap.h

LIBFILES = CommonDefs.o IoProcessor.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o CpAttr.o CpType.o CtlPkt.o \
	   QuManager.o PacketHeader.o PacketStore.o 

SUPPORT = support/timestamp

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

all : fHost fRouter Avatar Monitor
	cp fHost fRouter Avatar Monitor ${HOME}/bin

fRouter : RouterCore.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fHost : Host.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

Avatar : Avatar.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

Monitor : Monitor.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

lib : ${LIBFILES} ${SUPPORT}
	make -C support lib
	ar -ru lib ${LIBFILES}

clean :
	make -C support clean
	rm -f *.o lib fHost fRouter Avatar Monitor
