CC = g++
DD = ../../data_structures
CFLAGS = -pg -I${DD}

.c.o:
	${CC} ${CFLAGS} -c $*.c 

kruskal.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/partition.h
kruskal2.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/partition.h
kruskal3.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/partition.h
prim.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/heaps/dheap.h
prim2.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/heaps/dheap.h
primF.o : stdinc.h ${DD}/basic/clist.h ${DD}/basic/list.h \
		${DD}/basic/wgraph.h ${DD}/heaps/fheaps.h
primF2.o : stdinc.h ${DD}/basic/clist.h ${DD}/basic/list.h \
		${DD}/basic/wgraph.h ${DD}/heaps/fheaps.h
mstUpdate.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/list.h \
		${DD}/basic/clist.h ${DD}/basic/partition.h 
mstUpdate2.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/list.h \
		${DD}/basic/partition.h 
rrobin.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/list.h \
	${DD}/basic/dlist.h ${DD}/basic/partition.h ${DD}/heaps/llheaps.h
rrobin2.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/list.h \
	${DD}/basic/dlist.h ${DD}/basic/partition.h ${DD}/heaps/llheaps.h
check.o : stdinc.h ${DD}/basic/wgraph.h ${DD}/basic/list.h \
	${DD}/basic/clist.h ${DD}/basic/partition.h

all:	kruskal kruskal2 kruskal3 prim prim2 primF primF2 \
	rrobin rrobin2 check mstUpdate mstUpdate2

kruskal: kruskal.o ${DD}/basic/wgraph.o ${DD}/basic/partition.o
	${CC} ${CFLAGS} kruskal.o ${DD}/basic/wgraph.o \
	${DD}/basic/partition.o -o kruskal

kruskal2: kruskal2.o ${DD}/basic/wgraph.o ${DD}/basic/partition.o
	${CC} ${CFLAGS} kruskal2.o ${DD}/basic/wgraph.o \
	${DD}/basic/partition.o -o kruskal2

kruskal3: kruskal3.o ${DD}/basic/wgraph.o ${DD}/basic/partition.o
	${CC} ${CFLAGS} kruskal3.o ${DD}/basic/wgraph.o \
	${DD}/basic/partition.o -o kruskal3

prim: prim.o ${DD}/basic/wgraph.o ${DD}/heaps/dheap.o
	${CC} ${CFLAGS} prim.o ${DD}/basic/wgraph.o ${DD}/heaps/dheap.o \
	-o prim

prim2: prim2.o ${DD}/basic/wgraph.o ${DD}/heaps/dheap.o
	${CC} ${CFLAGS} prim2.o ${DD}/basic/wgraph.o ${DD}/heaps/dheap.o \
	-o prim2

primF: primF.o ${DD}/basic/wgraph.o ${DD}/basic/clist.o ${DD}/basic/list.o \
	${DD}/heaps/fheaps.o
	${CC} ${CFLAGS} primF.o ${DD}/basic/wgraph.o ${DD}/basic/clist.o \
	${DD}/basic/list.o ${DD}/heaps/fheaps.o -o primF

primF2: primF2.o ${DD}/basic/wgraph.o ${DD}/basic/clist.o ${DD}/basic/list.o \
	${DD}/heaps/fheaps.o
	${CC} ${CFLAGS} primF2.o ${DD}/basic/wgraph.o ${DD}/basic/clist.o \
	${DD}/basic/list.o ${DD}/heaps/fheaps.o -o primF2

rrobin: rrobin.o ${DD}/basic/wgraph.o ${DD}/basic/list.o ${DD}/basic/dlist.o \
	${DD}/basic/partition.o ${DD}/heaps/llheaps.o
	${CC} ${CFLAGS} rrobin.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/dlist.o ${DD}/basic/partition.o ${DD}/heaps/llheaps.o \
	-o rrobin

rrobin2: rrobin2.o ${DD}/basic/wgraph.o ${DD}/basic/list.o ${DD}/basic/dlist.o \
	${DD}/basic/partition.o ${DD}/heaps/llheaps.o
	${CC} ${CFLAGS} rrobin2.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/dlist.o ${DD}/basic/partition.o ${DD}/heaps/llheaps.o \
	-o rrobin2

check: check.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/clist.o ${DD}/basic/partition.o
	${CC} ${CFLAGS} check.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/clist.o ${DD}/basic/partition.o -o check

mstUpdate: mstUpdate.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/clist.o ${DD}/basic/partition.o
	${CC} ${CFLAGS} mstUpdate.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/clist.o ${DD}/basic/partition.o -o mstUpdate

mstUpdate2: mstUpdate2.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/partition.o
	${CC} ${CFLAGS} mstUpdate2.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/partition.o -o mstUpdate2

foo: foo.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/partition.o
	${CC} ${CFLAGS} foo.o ${DD}/basic/wgraph.o ${DD}/basic/list.o \
	${DD}/basic/partition.o ${DD}/basic/clist.o \
	${DD}/heaps/dheap.o -o foo
