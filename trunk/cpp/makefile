SHELL := /bin/bash
ALGOLIBROOT = /forest/src/algolib-jst
FORESTROOT = $(shell pwd)
FLIB = ${FORESTROOT}/lib/lib-forest.a
LIBS = ${FLIB} ${ALGOLIBROOT}/lib/lib-ds.a ${ALGOLIBROOT}/lib/lib-util.a
CXXFLAGS = -Wall -O2 -I ${FORESTROOT}/include -I ${ALGOLIBROOT}/include
#JAVAC=/usr/java/latest/bin/javac
JAVAC=javac
BIN = /forest/bin

all:
	make -C common          ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' JAVAC='${JAVAC}' all
	make -C router          ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' JAVAC='${JAVAC}' all
	make -C control         ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' JAVAC='${JAVAC}' all
	make -C vworld1         ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' JAVAC='${JAVAC}' all
	make -C misc            ALGOLIBROOT='${ALGOLIBROOT}' FLIB='${FLIB}' \
				BIN='${BIN}' CXXFLAGS='${CXXFLAGS}' \
				LIBS='${LIBS}' JAVAC='${JAVAC}' all

clean:
	rm -f ${FLIB} 
	make -C misc    clean
	make -C common  clean
	make -C router  clean
	make -C control clean
	make -C vworld1 clean
