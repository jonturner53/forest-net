IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
CXXFLAGS = -pg

.cpp.o:
	${CXX} ${CXXFLAGS} -I ${IDIR} -c $<

pathset_d.o : ${IDIR}/stdinc.h ${IDIR}/pathset.h
dtrees_d.o :  ${IDIR}/stdinc.h ${IDIR}/pathset.h ${IDIR}/dtrees.h
lmp_d.o :  ${IDIR}/stdinc.h ${IDIR}/lmp.h 

all:	pathset_d dtrees_d lmp_d

clean:
	rm -f *.o *.exe

pathset_d: pathset_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

dtrees_d: dtrees_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

lmp_d: lmp_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
