IDIR = ~/src/include
LDIR = ~/src/lib
LIB = ${LDIR}/lib-ds.a
CXXFLAGS = -pg
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

pathset.o : ${IDIR}/stdinc.h ${IDIR}/pathset.h
dtrees.o :  ${IDIR}/stdinc.h ${IDIR}/dtrees.h
lmp.o : ${IDIR}/stdinc.h ${IDIR}/lmp.h

all:	pathset.o dtrees.o lmp.o
	ar -ru ${LIB} pathset.o dtrees.o lmp.o
	make -C dvr IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
                    CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f *.o
	make -C dvr clean
