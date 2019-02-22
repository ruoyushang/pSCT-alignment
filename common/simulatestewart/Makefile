CXX=g++
CXXFLAGS=`root-config --cflags` -std=c++11
LIBS=`root-config --libs`

DEPS=stewartplatform.h
OBJ=simulatestewart.o stewartplatform.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

simulatestewart: $(OBJ)
	$(CXX) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f *.o simulatestewart
