BIN = ${HOME}/bin
IDIR = ../include
LDIR = ../lib
LIBS = ${LDIR}/libForest.a ${LDIR}/lib-misc.a ${LDIR}/lib-ds.a
LIBF = ${LDIR}/libForest.a
CXXFLAGS = -pg
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

all : fHost fRouter

HFILES = ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/forest.h ${IDIR}/fRouter.h \
	 ${IDIR}/ioProc.h ${IDIR}/lnkTbl.h ${IDIR}/comtTbl.h ${IDIR}/rteTbl.h \
	 ${IDIR}/statsMod.h ${IDIR}/qMgr.h ${IDIR}/pktStore.h ${IDIR}/Avatar.h \
	 ${IDIR}/Monitor.h

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
Avatar.o :   ${HFILES}
Monitor.o :   ${HFILES}

${LIBF} : lnkTbl.o comtTbl.o rteTbl.o pktStore.o qMgr.o ioProc.o \
	header.o statsMod.o Avatar.o Monitor.o
	ar -ru ${LIBF} lnkTbl.o comtTbl.o rteTbl.o pktStore.o qMgr.o \
	header.o ioProc.o statsMod.o Avatar.o Monitor.o

fRouter : fRouter.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp fRouter ${HOME}/bin

fLinecard : fLinecard.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

fHost : fHost.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp fHost ${HOME}/bin

Avatar : Avatar.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp Avatar ${HOME}/bin

Monitor : Monitor.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp Monitor ${HOME}/bin

clean :
	rm -f lnkTbl.o comtTbl.o rteTbl.o pktStore.o qMgr.o ioProc.o \
	statsMod.o fHost.o fRouter.o fLinecard.o Avatar.o Monitor.o ${LIBF} \
	fHost fRouter fLinecard Avatar Monitor
