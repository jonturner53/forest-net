IDIR = ~/src/include
LDIR = ~/src/lib
BIN = ~/bin
CXXFLAGS = -pg 

all:
	make -C misc            IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C dataStructures  IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C graphAlgorithms IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f ${LDIR}lib-misc.a ${LDIR}lib-ds.a ${LDIR}lib-ga.a
	make -C misc            clean
	make -C dataStructures  clean
	make -C graphAlgorithms clean

all:
	make -C misc   IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all
	make -C mst    IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all
	make -C sPath  IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all
	make -C maxFlo IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all
	make -C mcFlo  IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all
	make -C match  IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
		       CXXFLAGS='${CXXFLAGS}' all

clean:
	make -C misc   clean
	make -C mst    clean
	make -C sPath  clean
	make -C maxFlo clean
	make -C mcFlo  clean
	make -C match  clean
