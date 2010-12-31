BIN = ${HOME}/bin
IDIR = ~/src/include
LDIR = ~/src/lib
LIBWU = ${LIBDIR}/lib-wunet.a
LIBS = ${LIBDIR}/lib-wunet.a ${LIBDIR}/lib-misc.a ${LIBDIR}/lib-ds.a

CC = g++
CFLAGS = -O2 -I ${IDIR}
.c.o:
	${CC} ${CFLAGS} -c $*.c 

all : ${BIN}/wuHost ${BIN}/wuRouter ${BIN}/wuLinecard \
	${BIN}/wuHscript ${BIN}/wuRscript ${BIN}/wuLCscript

lnkTbl.o :     ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/lnkTbl.h
lcTbl.o :      ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/lcTbl.h
vnetTbl.o :    ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/vnetTbl.h
rteTbl.o :     ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/rteTbl.h
pktStore.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/pktStore.h
qMgr.o :       ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/qMgr.h
ioProc.o :     ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/ioProc.h
statsMod.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/statsMod.h
wuRouter.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/wuRouter.h
wuLinecard.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/wuLinecard.h
wuHost.o :     ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/wunet.h ${IDIR}/wuHost.h

${BIN}/wuHscript : wuHscript
	rm -f ${BIN}/wuHscript
	ln wuHscript ${BIN}

${BIN}/wuRscript : wuRscript
	rm -f ${BIN}/wuRscript
	ln wuRscript ${BIN}

${BIN}/wuLCscript : wuLCscript
	rm -f ${BIN}/wuLCscript
	ln wuLCscript ${BIN}

${BIN}/wuHost : wuHost
	rm -f ${BIN}/wuHost
	ln wuHost ${BIN}

${BIN}/wuRouter : wuRouter
	rm -f ${BIN}/wuRouter
	ln wuRouter ${BIN}

${BIN}/wuLinecard : wuLinecard
	rm -f ${BIN}/wuLinecard
	ln wuLinecard ${BIN}

${LIBWU} : lnkTbl.o vnetTbl.o rteTbl.o pktStore.o qMgr.o ioProc.o lcTbl.o statsMod.o
	ar -ru ${LIBWU} lnkTbl.o vnetTbl.o rteTbl.o pktStore.o qMgr.o lcTbl.o \
	ioProc.o statsMod.o

wuRouter : wuRouter.o ${LIBS}
	${CC} ${CFLAGS} wuRouter.o ${LIBS} -o wuRouter

wuLinecard : wuLinecard.o ${LIBS}
	${CC} ${CFLAGS} wuLinecard.o ${LIBS} -o wuLinecard

wuHost : wuHost.o ${LIBS}
	${CC} ${CFLAGS} wuHost.o ${LIBS} -o wuHost

clean :
	rm -f lnkTbl.o vnetTbl.o rteTbl.o pktStore.o qMgr.o ioProc.o \
	lcTbl.o statsMod.o ${BIN}/wuHscript ${BIN}/wuRscript ${BIN}/wuLCscript \
	${BIN}/wuHost ${BIN}/wuRouter ${BIN}/wuLinecard  \
	wuHost wuRouter wuLinecard wuHost.o wuRouter.o wuLinecard.o ${LIBS}
