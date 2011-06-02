CXXFLAGS = -O2 -m64 -I .. -I ../..

%.o : %.cpp
	${CXX} ${CXXFLAGS} -c $<

all : TestUiList TestUiDlist

TestUiList: TestUiList.o ../../lib
	${CXX} ${CXXFLAGS} $< ../../lib -o $@

TestUiDlist: TestUiDlist.o ../../lib
	${CXX} ${CXXFLAGS} $< ../../lib -o $@

clean :
	rm -f *.o
