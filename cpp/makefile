# Top level makefile, order of lower-level makes matters
SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
FROOT = $(shell pwd)
AROOT = ${FROOT}/../../algolib-jst
INCLUDES = -I ${FROOT}/include -I ${AROOT}/include
FLIB = ${FROOT}/lib/lib-forest.a
LIBS = ${FLIB} ${AROOT}/lib/lib-ds.a ${AROOT}/lib/lib-util.a
BIN = ~/bin
WARN = -Wall -Wno-c++11-extensions
CXXFLAGS = ${WARN} ${ARCH} ${INCLUDES} -O2 -std=c++0x
JAVAC = javac

all:
	make -C common          AROOT='${AROOT}' FROOT='${FROOT}' \
				ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C router          AROOT='${AROOT}' FROOT='${FROOT}' \
				ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C control         AROOT='${AROOT}' FROOT='${FROOT}' \
				ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				all
	make -C vworld1         AROOT='${AROOT}' FROOT='${FROOT}' \
				ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C misc         	AROOT='${AROOT}' FROOT='${FROOT}' \
				ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f ${FLIB} 
	make -C misc    clean
	make -C common  clean
	make -C router  clean
	make -C control clean
	make -C vworld1 clean
