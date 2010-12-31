IDIR = ~/src/include
LDIR = ~/src/lib
BIN = ~/bin
CXXFLAGS = -pg 


all:
	make -C basic       IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
			    CXXFLAGS='${CXXFLAGS}' all
	make -C graphs      IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
			    CXXFLAGS='${CXXFLAGS}' all
	make -C heaps       IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
			    CXXFLAGS='${CXXFLAGS}' all
	make -C searchTrees IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
			    CXXFLAGS='${CXXFLAGS}' all
	make -C advanced    IDIR='${IDIR}' LDIR='${LDIR}' BIN='${BIN}' \
			    CXXFLAGS='${CXXFLAGS}' all

clean:
	make -C basic       clean
	make -C graphs      clean
	make -C heaps       clean
	make -C searchTrees clean
	make -C advanced    clean
