CXXFLAGS = -O2 -m64
.cpp.o:
	${CXX} ${CXXFLAGS} -I support -c $<

HFILES = stdinc.h forest.h fRouter.h ioProc.h lnkTbl.h ComtTbl.h \
	 rteTbl.h statsMod.h qMgr.h CpAttr.h CpType.h CtlPkt.h \
	 header.h pktStore.h Avatar.h Monitor.h \
	 support/misc.h support/UiList.h support/UiDlist.h support/UiListSet.h \
	 support/UiHashTbl.h support/ModHeap.h

LIBFILES = forest.o ioProc.o lnkTbl.o ComtTbl.o rteTbl.o statsMod.o \
	 CpAttr.o CpType.o CtlPkt.o qMgr.o header.o pktStore.o 

SUPPORT = support/timestamp

lnkTbl.o :    ${HFILES}
ComtTbl.o :   ${HFILES}
rteTbl.o :    ${HFILES}
pktStore.o :  ${HFILES}
qMgr.o :      ${HFILES}
ioProc.o :    ${HFILES}
statsMod.o :  ${HFILES}
fRouter.o :   ${HFILES}
fLinecard.o : ${HFILES}
fHost.o :     ${HFILES}
Avatar.o :    ${HFILES}
Monitor.o :   ${HFILES}
CpAttr.o :    ${HFILES}
CpType.o :    ${HFILES}
CtlPkt.o :    ${HFILES}

all : fHost fRouter Avatar Monitor
	cp fHost fRouter Avatar Monitor ${HOME}/bin

fRouter : fRouter.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fHost : fHost.o lib
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
