CXXFLAGS = -O2 -m64 -I .. -I ../..
.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

OFILES = TestUiList.o TestUiDlist.o

TestUiList.o :    ${HFILES}
TestUiDlist.o :   ${HFILES}

all : TestUiList TestUiDlist

TestUiList: TestUiList.o ../../lib
	${CXX} ${CXXFLAGS} $< ../../lib -o $@

TestUiDlist: TestUiDlist.o ../../lib
	${CXX} ${CXXFLAGS} $< ../../lib -o $@

clean :
	rm -f *.o
