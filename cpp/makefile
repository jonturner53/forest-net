# Top level makefile, order of lower-level makes matters
SHELL := /bin/bash
ARCH := -m$(shell getconf LONG_BIT)
BIN := ~/bin
WARN := -Wall
CXXFLAGS := ${WARN} ${ARCH} -pthread -O2 -std=c++0x
JAVAC := javac

all:
	make -C common          ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all
	make -C mtrouter        ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all
#	make -C control         ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
#				all
#	make -C vworld1         ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
#				CXXFLAGS='${CXXFLAGS}' all
	make -C misc         	ARCH='${ARCH}' BIN='${BIN}' WARN='${WARN}' \
				CXXFLAGS='${CXXFLAGS}' all

clean:
	rm -f ${FLIB} 
	make -C misc    clean
	make -C common  clean
	make -C router  clean
	make -C control clean
	make -C vworld1 clean
