BIN = ${HOME}/bin
IDIR = ../include
LDIR = ../lib
LIBS = ${LDIR}/libLfs.a ${LDIR}/lib-misc.a ${LDIR}/lib-ds.a
LIBF = ${LDIR}/libLfs.a
CXXFLAGS = -O2
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -I . -c $<

all : lfsHost lfsRouter

HFILES = ${IDIR}/stdinc.h ${IDIR}/misc.h lfs.h lfsRouter.h ioProc.h \
	 lnkTbl.h fltrTbl.h rteTbl.h statsMod.h qMgr.h pktStore.h header.h

lnkTbl.o :    ${HFILES}
fltrTbl.o :   ${HFILES}
rteTbl.o :    ${HFILES}
pktStore.o :  ${HFILES}
header.o :    ${HFILES}
qMgr.o :      ${HFILES}
ioProc.o :    ${HFILES}
statsMod.o :  ${HFILES}
lfsRouter.o :   ${HFILES}
fLinecard.o : ${HFILES}
lfsHost.o :     ${HFILES}

${LIBF} : lnkTbl.o fltrTbl.o rteTbl.o pktStore.o header.o qMgr.o ioProc.o \
	statsMod.o
	ar -ru ${LIBF} lnkTbl.o fltrTbl.o rteTbl.o pktStore.o header.o qMgr.o \
	ioProc.o statsMod.o

lfsRouter : lfsRouter.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp lfsRouter ${HOME}/bin

lfsHost : lfsHost.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp lfsHost ${HOME}/bin

clean :
	rm -f lnkTbl.o fltrTbl.o rteTbl.o pktStore.o qMgr.o ioProc.o \
	lcTbl.o statsMod.o lfsHost.o lfsRouter.o fLinecard.o ${LIBF} \
	lfsHost lfsRouter *.exe
