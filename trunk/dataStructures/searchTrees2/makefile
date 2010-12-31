IDIR = ~/src/include
LDIR = ~/src/lib
LIB = ${LDIR}/lib-ds.a
CXXFLAGS = -pg
.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

ssets.o : ${IDIR}/stdinc.h ${IDIR}/ssets.h
bssets.o : ${IDIR}/stdinc.h ${IDIR}/bssets.h
sass.o :  ${IDIR}/stdinc.h ${IDIR}/sass.h ${IDIR}/ssets.h
dkst.o :  ${IDIR}/stdinc.h ${IDIR}/dkst.h ${IDIR}/sass.h ${IDIR}/ssets.h
steps.o : ${IDIR}/stdinc.h ${IDIR}/steps.h ${IDIR}/dkst.h ${IDIR}/sass.h \
	  ${IDIR}/ssets.h

all:	ssets.o bssets.o sass.o dkst.o steps.o
	ar -ru ${LIB} ssets.o bssets.o sass.o dkst.o steps.o
	make -C dvr IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
                    CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f *.o
	make -C dvr clean
