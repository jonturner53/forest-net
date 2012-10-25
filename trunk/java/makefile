SHELL := /bin/bash
JAVAC=javac
BIN = ~/bin

all:
	make -C common          BIN='${BIN}' JAVAC='${JAVAC}' all
	make -C control          BIN='${BIN}' JAVAC='${JAVAC}' all
	make -C vworld1          BIN='${BIN}' JAVAC='${JAVAC}' all

clean:
	make -C common  clean
	make -C control clean
	make -C vworld1 clean
