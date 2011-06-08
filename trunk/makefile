CXXFLAGS = -O2 -m64 -I . -I support

LIBS = lib support/lib

%.o: %.cpp %.h stdinc.h
	${CXX} ${CXXFLAGS} -c $<

LIBFILES = CommonDefs.o IoProcessor.o LinkTable.o ComtreeTable.o \
	   RouteTable.o StatsModule.o CpAttr.o CpType.o CtlPkt.o \
	   QuManager.o PacketHeader.o PacketStore.o 

all : supportLib fHost fRouter fAvatar fMonitor fNetMgr

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

fMonitor : Monitor.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

fNetMgr : NetMgr.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
	cp $@ ${HOME}/bin

lib : ${LIBFILES}
	ar -ru lib ${LIBFILES}

clean :
	${MAKE} -C support clean
	rm -f *.o lib fHost fRouter fAvatar fMonitor fNetMgr
