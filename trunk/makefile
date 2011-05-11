CXXFLAGS = -O2
.cpp.o:
	${CXX} ${CXXFLAGS} -I support -c $<

HFILES = stdinc.h forest.h fRouter.h ioProc.h lnkTbl.h comtTbl.h \
	 rteTbl.h statsMod.h qMgr.h CpAttr.h CpType.h CtlPkt.h \
	 header.h pktStore.h Avatar.h Monitor.h \
	 support/misc.h support/list.h support/dlist.h support/listset.h \
	 support/hashTbl.h support/mheap.h

LIBFILES = forest.o ioProc.o lnkTbl.o comtTbl.o rteTbl.o statsMod.o \
	 CpAttr.o CpType.o CtlPkt.o qMgr.o header.o pktStore.o \
	 support/misc.o support/list.o support/dlist.o support/listset.o \
	 support/hashTbl.o support/mheap.o

lnkTbl.o :    ${HFILES}
comtTbl.o :   ${HFILES}
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

lib : ${LIBFILES}
	make -C support
	ar -ru lib ${LIBFILES}

all : fHost fRouter Avatar Monitor

fRouter : fRouter.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

fHost : fHost.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

Avatar : Avatar.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

Monitor : Monitor.o lib
	${CXX} ${CXXFLAGS} $< lib -o $@

clean :
	make -C support
	rm -f *.o lib fHost fRouter Avatar Monitor
