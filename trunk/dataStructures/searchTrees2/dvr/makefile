IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

ssets_d.o :  ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/ssets.h
bssets_d.o : ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/bssets.h
sass_d.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/sass.h
dkst_d.o :   ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/dkst.h
steps_d.o :  ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/steps.h
stepsCnt.o :  ${IDIR}/stdinc.h ${IDIR}/misc.h ${IDIR}/steps.h

all:	ssets_d bssets_d sass_d dkst_d steps_d stepsCnt

clean:
	rm -f *.o *.exe

ssets_d: ssets_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

bssets_d: bssets_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

sass_d: sass_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

dkst_d: dkst_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

steps_d: steps_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

stepsCnt: stepsCnt.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
