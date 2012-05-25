SHELL := /bin/bash
ALGOLIBROOT = ~/src/algolib-jst
FLIB = $(shell pwd)/lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
CXXFLAGS = -O2 -m64 -I ../include -I ${ALGOLIBROOT}/include
BIN = ~/bin

all:
	make -C common          ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' all
	make -C router          ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' all
	make -C control         ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' all
	make -C vworld1         ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' all
	make -C misc            ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' all

clean:
	rm -f ${FLIB} 
	make -C misc    clean
	make -C common  clean
	make -C router  clean
	make -C control clean
	make -C vworld1 clean
