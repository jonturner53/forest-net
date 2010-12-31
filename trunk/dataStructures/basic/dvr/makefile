IDIR = ~/src/include
LDIR = ~/src/lib
LIBS = ${LDIR}/lib-ds.a ${LDIR}/lib-misc.a
CXXFLAGS = -pg

.cpp.o:
	$(CXX) $(CXXFLAGS) -I ${IDIR} -c $<

list_d.o :    ${IDIR}/stdinc.h ${IDIR}/list.h  ${IDIR}/misc.h
lists_d.o :   ${IDIR}/stdinc.h ${IDIR}/lists.h  ${IDIR}/misc.h
dlist_d.o :   ${IDIR}/stdinc.h ${IDIR}/list.h ${IDIR}/dlist.h  ${IDIR}/misc.h
clist_d.o :   ${IDIR}/stdinc.h ${IDIR}/clist.h  ${IDIR}/misc.h
rlist_d.o :   ${IDIR}/stdinc.h ${IDIR}/rlist.h  ${IDIR}/misc.h
listset_d.o : ${IDIR}/stdinc.h ${IDIR}/listset.h  ${IDIR}/misc.h
prtn_d.o :    ${IDIR}/stdinc.h ${IDIR}/prtn.h  ${IDIR}/misc.h
hashTbl_d.o : ${IDIR}/stdinc.h ${IDIR}/hashTbl.h  ${IDIR}/misc.h

all:	list_d lists_d dlist_d clist_d rlist_d listset_d prtn_d hashTbl_d

clean:
	rm -f *.o *.exe

list_d: list_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

lists_d: lists_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

dlist_d: dlist_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

clist_d: clist_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

rlist_d: rlist_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

listset_d: listset_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

prtn_d: prtn_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@

hashTbl_d: hashTbl_d.o ${LIBS}
	${CXX} ${CXXFLAGS} $< ${LIBS} -o $@
