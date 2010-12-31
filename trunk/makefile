# Top level makefile, order of lower-level makes matters
SHELL := /bin/bash
IDIR = $(shell pwd)/include
LDIR = $(shell pwd)/lib
BIN = ~/bin
CXXFLAGS = -O2

all:
	make -C misc            IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C dataStructures  IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C graphAlgorithms IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C forest		IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C lfs		IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
				CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f ${LDIR}/lib-misc.a ${LDIR}/lib-ds.a ${LDIR}/lib-ga.a \
		${LDIR}/libForest.a
	make -C misc            clean
	make -C dataStructures  clean
	make -C graphAlgorithms clean
	make -C forest		clean
	make -C lfs		clean

